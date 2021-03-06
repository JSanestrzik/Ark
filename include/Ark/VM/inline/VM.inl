// ------------------------------------------
//               instructions
// ------------------------------------------

#define createNewScope() m_locals.emplace_back(std::make_shared<internal::Scope>());

template <typename... Args>
internal::Value VM::call(const std::string& name, Args&&... args)
{
    using namespace Ark::internal;

    const std::lock_guard<std::mutex> lock(m_mutex);

    // reset ip and pp
    m_ip = 0;
    m_pp = 0;

    // find id of function
    auto it = std::find(m_state->m_symbols.begin(), m_state->m_symbols.end(), name);
    if (it == m_state->m_symbols.end())
        throwVMError("unbound variable: " + name);

    // convert and push arguments in reverse order
    std::vector<Value> fnargs { { Value(args)... } };
    for (auto it2=fnargs.rbegin(), it_end=fnargs.rend(); it2 != it_end; ++it2)
        m_frames.back().push(*it2);

    // find function object and push it if it's a pageaddr/closure
    uint16_t id = static_cast<uint16_t>(std::distance(m_state->m_symbols.begin(), it));
    auto var = findNearestVariable(id);
    if (var != nullptr)
    {
        if (var->valueType() != ValueType::PageAddr && var->valueType() != ValueType::Closure)
            throwVMError("Can't call '" + name + "': it isn't a Function but a " + types_to_str[static_cast<int>(var->valueType())]);

        m_frames.back().push(*var);
        m_last_sym_loaded = id;
    }
    else
        throwVMError("Couldn't find variable " + name);

    std::size_t frames_count = m_frames.size();
    // call it
    call(static_cast<int16_t>(sizeof...(Args)));
    // reset instruction pointer, otherwise the safeRun method will start at ip = -1
    // without doing m_ip++ as intended (done right after the call() in the loop, but here
    // we start outside this loop)
    m_ip = 0;

    // run until the function returns
    safeRun(/* untilFrameCount */ frames_count);

    // get result
    if (m_frames.back().stackSize() != 0)
        return *m_frames.back().pop();
    else
        return Builtins::nil;
}

inline internal::Value* VM::findNearestVariable(uint16_t id) noexcept
{
    for (auto it=m_locals.rbegin(), it_end=m_locals.rend(); it != it_end; ++it)
    {
        if (auto val = (**it)[id]; val != nullptr)
            return val;
    }
    return nullptr;
}

inline void VM::returnFromFuncCall()
{
    // remove frame
    m_frames.pop_back();
    uint8_t del_counter = m_frames.back().scopeCountToDelete();

    // high cpu cost because destroying variants cost
    m_locals.pop_back();

    while (del_counter != 0)
    {
        m_locals.pop_back();
        del_counter--;
    }

    m_frames.back().resetScopeCountToDelete();

    // stop the executing if we reach the wanted frame count
    if (m_frames.size() == m_until_frame_count)
        m_running = false;
}

inline void VM::call(int16_t argc_)
{
    /*
        Argument: number of arguments when calling the function
        Job: Call function from its symbol id located on top of the stack. Take the given number of
                arguments from the top of stack and give them  to the function (the first argument taken
                from the stack will be the last one of the function). The stack of the function is now composed
                of its arguments, from the first to the last one
    */
    using namespace Ark::internal;

    uint16_t argc = 0;

    // handling calls from C++ code
    if (argc_ <= -1)
    {
        ++m_ip;
        argc = (static_cast<uint16_t>(m_state->m_pages[m_pp][m_ip]) << 8) + static_cast<uint16_t>(m_state->m_pages[m_pp][m_ip + 1]);
        ++m_ip;
    }
    else
        argc = argc_;

    Value function = *m_frames.back().pop();

    switch (function.valueType())
    {
        // is it a builtin function name?
        case ValueType::CProc:
        {
            // drop arguments from the stack
            std::vector<Value> args(argc);
            for (uint16_t j=0; j < argc; ++j)
                args[argc - 1 - j] = *m_frames.back().pop();

            // call proc
            m_frames.back().push(function.proc()(args, this));
            return;
        }

        // is it a user defined function?
        case ValueType::PageAddr:
        {
            int old_frame = static_cast<int>(m_frames.size()) - 1;
            PageAddr_t new_page_pointer = function.pageAddr();

            // create dedicated frame
            createNewScope();
            m_frames.emplace_back(m_ip, static_cast<uint16_t>(m_pp), new_page_pointer);
            // store "reference" to the function to speed the recursive functions
            if (m_last_sym_loaded < m_state->m_symbols.size())
                m_locals.back()->push_back(m_last_sym_loaded, function);

            m_pp = new_page_pointer;
            m_ip = -1;  // because we are doing a m_ip++ right after that
            for (std::size_t j=0; j < argc; ++j)
                m_frames.back().push(*m_frames[old_frame].pop());
            break;
        }

        // is it a user defined closure?
        case ValueType::Closure:
        {
            int old_frame = static_cast<int>(m_frames.size()) - 1;
            Closure& c = function.closure_ref();
            PageAddr_t new_page_pointer = c.pageAddr();

            // load saved scope
            m_locals.push_back(c.scope());
            // create dedicated frame
            createNewScope();
            m_frames.back().incScopeCountToDelete();
            m_frames.emplace_back(m_ip, static_cast<uint16_t>(m_pp), new_page_pointer);

            m_pp = new_page_pointer;
            m_ip = -1;  // because we are doing a m_ip++ right after that
            for (std::size_t j=0; j < argc; ++j)
                m_frames.back().push(*m_frames[old_frame].pop());
            break;
        }

        default:
            throwVMError("Can't call '" + m_state->m_symbols[m_last_sym_loaded] + "': it isn't a Function but a " + types_to_str[static_cast<int>(function.valueType())]);
    }

    // checking function arity
    if (m_state->m_options & FeatureFunctionArityCheck)
    {
        std::size_t index = 0,
                    needed_argc = 0,
                    received_argc = m_frames.back().stackSize();

        // every argument is a MUT declaration in the bytecode
        while (m_state->m_pages[m_pp][index] == Instruction::MUT)
        {
            needed_argc += 1;
            index += 3;  // jump the argument of MUT (integer on 2 bits, big endian)
        }

        if (needed_argc != received_argc)
            throwVMError("Function '" + m_state->m_symbols[m_last_sym_loaded] + "' needs " + Ark::Utils::toString(needed_argc) + " arguments, but it received " + Ark::Utils::toString(received_argc));
    }
}

template <typename... Args>
internal::Value VM::resolve(const internal::Value* val, Args&&... args)
{
    using namespace Ark::internal;

    const std::lock_guard<std::mutex> lock(m_mutex);

    if (!val->isFunction())
        throw Ark::TypeError("Value::resolve couldn't resolve a non-function");

    int ip = m_ip;
    std::size_t pp = m_pp;

    // convert and push arguments in reverse order
    std::vector<Value> fnargs { { Value(args)... } };
    for (auto it=fnargs.rbegin(), it_end=fnargs.rend(); it != it_end; ++it)
        m_frames.back().push(*it);
    // push function
    m_frames.back().push(*val);

    std::size_t frames_count = m_frames.size();
    // call it
    call(static_cast<int16_t>(sizeof...(Args)));
    // reset instruction pointer, otherwise the safeRun method will start at ip = -1
    // without doing m_ip++ as intended (done right after the call() in the loop, but here
    // we start outside this loop)
    m_ip = 0;

    // run until the function returns
    safeRun(/* untilFrameCount */ frames_count);

    // restore VM state
    m_ip = ip;
    m_pp = pp;

    // get result
    if (m_frames.back().stackSize() != 0)
        return *m_frames.back().pop();
    else
        return Builtins::nil;
}

#undef createNewScope