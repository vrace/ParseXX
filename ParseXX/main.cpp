#include <iostream>
#include <functional>
#include <ostream>
#include <cstdlib>

template <class T>
struct Optional
{
    T data;
    bool hasValue;
    
    Optional(T value)
        : data(value)
        , hasValue(true)
    {
    }
    
    Optional()
        : data()
        , hasValue(false)
    {
    }
    
    template <class U>
    Optional<U> map(std::function<U (T)> f)
    {
        if (!hasValue)
            return Optional<U>();
        
        return Optional<U>(f(data));
    }
    
    template <class U>
    Optional<U> flatMap(std::function<Optional<U> (T)> f)
    {
        if (!hasValue)
            return Optional<U>();
        
        return f(data);
    }
};

template <class T>
std::ostream& operator <<(std::ostream &s, const Optional<T> &data)
{
    if (!data.hasValue)
    {
        s << "nil";
    }
    else
    {
        s << "some(" << data.data << ")";
    }
    
    return s;
}

enum class Gender
{
    Male,
    Female,
};

std::ostream& operator <<(std::ostream &s, const Gender &gender)
{
    if (gender == Gender::Male)
    {
        s << "(Male)";
    }
    else if (gender == Gender::Female)
    {
        s << "(Female)";
    }
    
    return s;
}

template <class T, class X = std::string>
struct Parser
{
    using FromType = X;
    using TargetType = T;
    using ReturnType = Optional<T>;
    using Type = std::function<ReturnType (const X&)>;
    
    Type parse;
    
    Parser(Type fn)
        : parse(fn)
    {
    }
    
    template <class U>
    Parser<U, X> map(std::function<U (T)> f)
    {
        return Parser<U, X>([this, f](const X &input) -> Optional<U> {
            return this->parse(input).map(f);
        });
    }
    
    template <class U>
    Parser<U, X> flatMap(std::function<Parser<U,X> (T)> fn)
    {
        return Parser<U, X>([this, fn](const X &input) -> Optional<U> {
            return this->parse(input).template flatMap<U>([fn, input](const T &temp) -> Optional<U> {
                return fn(temp).parse(input);
            });
        });
    }
    
    static Parser<T, X> unit(T value)
    {
        return Parser<T, X>([value](const X&) -> Optional<T> {
            return value;
        });
    }
    
    static Parser<T, X> fail()
    {
        return Parser<T, X>([](const X&) -> Optional<T> {
            return Optional<T>();
        });
    }
    
    Parser<T, X> operator | (const Parser<T, X> &rhs)
    {
        return Parser<T, X>([this, &rhs](const X &input) -> Optional<T> {
            auto a = this->parse(input);
            return a.hasValue ? a : rhs.parse(input);
        });
    }
};

int numberToInt(double value)
{
    return (int)value;
}

template <class U, class T, class X = std::string>
Parser<U, X> parse(Parser<T, X> parser, T input, U as)
{
    return parser.template flatMap<U>([input, as](const X &temp) -> Parser<U, X> {
        return temp == input ? Parser<U, X>::unit(as) : Parser<U, X>::fail();
    });
}

int main(void)
{
    auto stringParser = Parser<std::string>([](const std::string &input) -> Optional<std::string> {
        return Optional<std::string>(input);
    });
    
    auto numberParser = Parser<double>([](const std::string &input) -> Optional<double> {
        char *end;
        double d = strtod(input.c_str(), &end);
        if (*end)
            return Optional<double>();
        
        return Optional<double>(d);
    });
    
    auto intParser = numberParser.map<int>(numberToInt);
    
    auto maleParser = parse<Gender, std::string>(stringParser, "male", Gender::Male);
    auto femaleParser = parse<Gender, std::string>(stringParser, "female", Gender::Female);
    auto dudeParser = parse<Gender, std::string>(stringParser, "dude", Gender::Male);
    
    auto genderParser = maleParser | femaleParser | dudeParser;
    
    std::cout << stringParser.parse("123") << std::endl;
    std::cout << numberParser.parse("123.321") << std::endl;
    std::cout << numberParser.parse("haha") << std::endl;
    std::cout << intParser.parse("123.321") << std::endl;
    std::cout << genderParser.parse("male") << std::endl;
    std::cout << genderParser.parse("female") << std::endl;
    std::cout << genderParser.parse("xxx") << std::endl;
    
    return 0;
}

