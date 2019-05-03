#include <Ark/Lang/Node.hpp>

#include <Ark/Lang/Environment.hpp>

namespace Ark
{
    namespace Lang
    {
        Node::Node(int value) :
            m_type(NodeType::Number),
            m_value(BigNum(value)),
            m_env(nullptr)
        {}

        Node::Node(const BigNum& value) :
            m_type(NodeType::Number),
            m_value(value),
            m_env(nullptr)
        {}
        
        Node::Node(const std::string& value) :
            m_type(NodeType::String),
            m_value(value),
            m_env(nullptr)
        {}

        Node::Node(NodeType type) :
            m_type(type), m_env(nullptr)
        {}

        Node::Node(NodeType type, int value) :
            m_type(type),
            m_value(BigNum(value)),
            m_env(nullptr)
        {}

        Node::Node(NodeType type, const BigNum& value) :
            m_type(type),
            m_value(value),
            m_env(nullptr)
        {}

        Node::Node(NodeType type, const std::string& value) :
            m_type(type),
            m_value(value),
            m_env(nullptr)
        {}

        Node::Node(NodeType type, Keyword value) :
            m_type(type),
            m_value(BigNum(0)),
            m_keyword(value),
            m_env(nullptr)
        {}

        Node::Node(Node::ProcType proc) :
            m_procedure(proc),
            m_type(NodeType::Proc),
            m_env(nullptr)
        {}

        void Node::addEnv(Environment* env)
        {
            m_env = env;
        }

        Environment* Node::getEnv()
        {
            return m_env;
        }

        const std::string& Node::getStringVal() const
        {
            return std::get<std::string>(m_value);
        }

        const BigNum Node::getIntVal() const
        {
            return std::get<BigNum>(m_value);
        }
        
        const Node::ProcType Node::getProcVal() const
        {
            return m_procedure;
        }

        void Node::push_back(const Node& node)
        {
            m_list.push_back(node);
        }

        const NodeType& Node::nodeType() const
        {
            return m_type;
        }

        void Node::setNodeType(NodeType type)
        {
            m_type = type;
        }

        const Keyword Node::keyword() const
        {
            return m_keyword;
        }

        void Node::setKeyword(Keyword kw)
        {
            m_keyword = kw;
        }

        std::vector<Node>& Node::list()
        {
            return m_list;
        }

        const std::vector<Node>& Node::const_list() const
        {
            return m_list;
        }

        Node Node::call(const std::vector<Node>& args)
        {
            return m_procedure(args);
        }

        void Node::setPos(std::size_t line, std::size_t col)
        {
            m_line = line;
            m_col = col;
        }

        std::size_t Node::line() const
        {
            return m_line;
        }
        
        std::size_t Node::col() const
        {
            return m_col;
        }

        std::ostream& operator<<(std::ostream& os, const Node& N)
        {
            switch(N.m_type)
            {
            case NodeType::String:
            case NodeType::Symbol:
                os << std::get<std::string>(N.m_value);
                break;

            case NodeType::Number:
                os << std::get<BigNum>(N.m_value);
                break;

            case NodeType::List:
            {
                os << "( ";
                for (auto& t: N.m_list)
                    os << t << " ";
                os << ")";
                break;
            }

            case NodeType::Proc:
                os << "Procedure";
                break;

            case NodeType::Closure:
                os << "Closure";
                break;

            default:
                os << "~\\._./~";
                break;
            }
            return os;
        }

        extern const Node nil = Node(NodeType::Symbol, std::string("nil"));
        extern const Node falseSym = Node(NodeType::Symbol, std::string("false"));
        extern const Node trueSym = Node(NodeType::Symbol, std::string("true"));
    }
}
