#ifndef ark_ark

#include <chrono>
#include <iostream>

#include <clipp.hpp>
#include <Ark/Ark.hpp>
#include <Ark/REPL/Repl.hpp>

void bcr(const std::string& file)
{
    try {
        Ark::BytecodeReader bcr;
        bcr.feed(file);
        bcr.display();
    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
    }
}

int main(int argc, char** argv)
{
    using namespace clipp;

    enum class mode { help, dev_info, bytecode_reader, version, run, repl, compile, eval };
    mode selected = mode::repl;

    std::string file = "", lib_dir = "?", eval_expresion = "";
    unsigned debug = 0;
    std::vector<std::string> wrong, script_args;
    uint16_t options = Ark::DefaultFeatures;

    auto cli = (
        option("-h", "--help").set(selected, mode::help).doc("Display this message")
        | option("-v", "--version").set(selected, mode::version).doc("Display ArkScript version and exit")
        | option("--dev-info").set(selected, mode::dev_info).doc("Display development information and exit")
        | (
            required("-e", "--eval").set(selected, mode::eval).doc("Evaluate ArkScript expression")
            & value("expression", eval_expresion)
        )
        | (
            required("-c", "--compile").set(selected, mode::compile).doc("Compile the given program to bytecode, but do not run")
            & value("file", file)
            , joinable(repeatable(option("-d", "--debug").call([&]{ debug++; }).doc("Increase debug level (default: 0)")))
        )
        | (
            required("-bcr", "--bytecode-reader").set(selected, mode::bytecode_reader).doc("Launch the bytecode reader")
            & value("file", file)
        )
        | (
            value("file", file).set(selected, mode::run)
            , (
                joinable(repeatable(option("-d", "--debug").call([&]{ debug++; })))
                ,
                // shouldn't change now, the lib option is fine and working
                (
                    option("-L", "--lib").doc("Set the location of the ArkScript standard library")
                    & value("lib_dir", lib_dir)
                ),
                // feature flags
                with_prefix("-f",
                    // a single feature should always be defined with an ON and an OFF version, and documentation
                    // all features must be separated by commas as following
                    // exclusing feature flags (ON/OFF) should be separated by pipes
                    ( option("fac"   ).call([&]{ options |= Ark::FeatureFunctionArityCheck; })
                    | option("no-fac").call([&]{ options &= ~Ark::FeatureFunctionArityCheck; })
                    ).doc("Toggle function arity checks (default: ON)")
                    ,
                    ( option("ruv"   ).call([&]{ options |= Ark::FeatureRemoveUnusedVars; })
                    | option("no-ruv").call([&]{ options &= ~Ark::FeatureRemoveUnusedVars; })
                    ).doc("Remove unused variables (default: ON)")
                )
            )
            , any_other(script_args)
        )
        , any_other(wrong)
    );

    auto fmt = doc_formatting{}
        .start_column(8)           // column where usage lines and documentation starts
        .doc_column(36)            // parameter docstring start col
        .indent_size(2)            // indent of documentation lines for children of a documented group
        .split_alternatives(true)  // split usage into several lines for large alternatives
        .merge_alternative_flags_with_common_prefix(true)  // [-fok] [-fno-ok] becomes [-f(ok|no-ok)]
    ;

    if (parse(argc, argv, cli) && wrong.empty())
    {
        using namespace Ark;

        switch (selected)
        {
            case mode::help:
                std::cout << make_man_page(cli, "ark", fmt)
                            .prepend_section("DESCRIPTION",
                                             "        ArkScript programming language")
                            .append_section("LICENSE",
                                            "        Mozilla Public License 2.0");
                std::cout << std::endl;
                break;

            case mode::version:
                std::cout << "Version " << ARK_VERSION_MAJOR << "." << ARK_VERSION_MINOR << "." << ARK_VERSION_PATCH << std::endl;
                break;

            case mode::dev_info:
                std::cout << "Have been compiled with " << ARK_COMPILER << ", options: " << ARK_COMPILATION_OPTIONS << "\n\n";
                std::cout << "sizeof(Ark::Value)    = " << sizeof(Ark::internal::Value) << "B\n";
                std::cout << "      sizeof(Value_t) = " << sizeof(Ark::internal::Value::Value_t) << "B\n";
                std::cout << "      sizeof(ValueType) = " << sizeof(Ark::internal::ValueType) << "B\n";
                std::cout << "      sizeof(ProcType)  = " << sizeof(Ark::internal::Value::ProcType) << "B\n";
                std::cout << "sizeof(Ark::Frame)    = " << sizeof(Ark::internal::Frame) << "B\n";
                std::cout << "sizeof(Ark::State)    = " << sizeof(Ark::State) << "B\n";
                std::cout << "sizeof(Ark::Closure)  = " << sizeof(Ark::internal::Closure) << "B\n";
                std::cout << "sizeof(Ark::UserType) = " << sizeof(Ark::UserType) << "B\n";
                std::cout << "sizeof(Ark::VM)       = " << sizeof(Ark::VM) << "B\n";
                std::cout << "sizeof(vector<Ark::Value>) = " << sizeof(std::vector<Ark::internal::Value>) << "B\n";
                std::cout << "sizeof(std::string)   = " << sizeof(std::string) << "B\n";
                std::cout << "sizeof(String)        = " << sizeof(String) << "B\n";
                std::cout << "sizeof(char)          = " << sizeof(char) << "B\n";
                std::cout << std::endl;
                break;

            case mode::repl:
            {
                // send default features without FeatureRemoveUnusedVars to avoid deleting code which will be used later on
                Ark::Repl repl(Ark::DefaultFeatures & ~Ark::FeatureRemoveUnusedVars, lib_dir);
                repl.run();
                break;
            }

            case mode::compile:
            {
                Ark::State state(options, lib_dir);
                state.setDebug(debug);

                if (!state.doFile(file))
                {
                    Ark::logger.error("Ark::State.doFile(" + file + ") failed");
                    return -1;
                }

                break;
            }

            case mode::run:
            {
                Ark::State state(options, lib_dir);
                state.setDebug(debug);
                state.setArgs(script_args);

                if (!state.doFile(file))
                {
                    Ark::logger.error("Ark::State.doFile(" + file + ") failed");
                    return -1;
                }

                Ark::VM vm(&state);
                return vm.run();
            }

            case mode::eval:
            {
                Ark::State state(options, lib_dir);
                state.setDebug(debug);

                if (!state.doString(eval_expresion))
                {
                    Ark::logger.error("Ark::State.doString(" + eval_expresion + ") failed");
                    return  -1;
                }

                Ark::VM vm(&state);
                return  vm.run();
            }

            case mode::bytecode_reader:
                bcr(file);
                break;
        }
    }
    else
    {
        for (const auto& arg : wrong)
            std::cout << "'" << arg << "'" << " ins't a valid argument" << std::endl;

        std::cout << make_man_page(cli, "ark", fmt)
                    .prepend_section("DESCRIPTION",
                                        "        ArkScript programming language")
                    .append_section("LICENSE",
                                    "        Mozilla Public License 2.0");
        std::cout << std::endl;
    }

    // to avoid some "CLI glitches"
    std::cout << termcolor::reset;
    return 0;
}

#endif
