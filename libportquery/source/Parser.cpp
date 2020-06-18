#include "Parser.h"

#include <algorithm>

// helper class for std::visit. Constructs a callable which accepts various types based on deduction
template<typename ... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<typename ... Ts> overloaded(Ts...) -> overloaded<Ts...>;

template<typename ... Ts> bool MATCH(const Token t) { return (std::holds_alternative<Ts>(t) || ...); }

/*
// Note the wierd syntax in the example above, this is because you are looking a braces initialized constructor
template <typename T> struct isElementPresent: private std::vector<T> {
    using std::vector<T>::vector;
    bool operator==(const char& c) const {
        // Upcast to so that we can perform std::any_of on our vector
        const std::vector<T>& collection = static_cast<const std::vector<T>&>(*this);
        return std::any_of(collection.cbegin(), collection.cend(), [&c](const T& other) {return c == other;});
    }

    // Add this is so we can support both comparisons from both sides
    friend bool operator==(const T& lhs, const isElementPresent rhs) { return rhs == lhs; }
};
*/

std::string getTokenString(const Token t) {

    return std::visit(overloaded {
                [=] (NumericToken n)  { return std::to_string(n.m_value); },
                [=] (ComparisonToken) { return std::string("[COMPARISON TOKEN]"); },
                [=] (UserToken u)     { return u.m_UserToken; },
                [=] (ALLToken)        { return std::string("[ALL KEYWORD]"); },
                [=] (ANDToken)        { return std::string("AND KEYWORD]"); },
                [=] (ANYToken)        { return std::string("[ANY KEYWORD]"); },
                [=] (BETWEENToken)    { return std::string("[BETWEEN KEYWORD]"); },
                [=] (COUNTToken)      { return std::string("[COUNT KEYWORD]"); },
                [=] (FROMToken)       { return std::string("[FROM KEYWORD]"); },
                [=] (IFToken)         { return std::string("[IF KEYWORD]"); },
                [=] (INToken)         { return std::string("[IN KEYWORD]"); },
                [=] (ISToken)         { return std::string("[IS KEYWORD]"); },
                [=] (LIMITToken)      { return std::string("[LIMIT KEYWORD]"); },
                [=] (NOTToken)        { return std::string("[NOT KEYWORD]"); },
                [=] (ORToken)         { return std::string("[OR KEYWORD]"); },
                [=] (ORDERToken)      { return std::string("[ORDER KEYWORD]"); },
                [=] (SELECTToken)     { return std::string("[SELECT KEYWORD]");},
                [=] (WHEREToken)      { return std::string("[WHERE KEYWORD]"); },
                [=] (ColumnToken)   { return std::string("[PROTOCOL TOKEN]"); }, // more granularity please
                [=] (EOFToken)        { return std::string("[END OF INPUT]"); },
                [=] (PunctuationToken<'*'>) { return std::string("[ * ]"); },
                [=] (PunctuationToken<'('>) { return std::string("[ ( ]"); }, 
                [=] (PunctuationToken<')'>) { return std::string("[ ) ]"); },
                [=] (PunctuationToken<';'>) { return std::string("[ ; ]"); }, 
                [=] (PunctuationToken<','>) { return std::string("[ , ]"); },
                [=] (auto) -> std::string { return std::string("[UNKNOWN TOKEN]"); } }, 
            t);
    }


SOSQLSelectStatement Parser::parseSOSQLStatement() {

    // SOSQL statements can obviously only begin with the "SELECT" keyword
    if (!MATCH<SELECTToken>(m_lexer.nextToken())) {

        throw std::invalid_argument("Only SELECT statements are handled. Statement must begin with SELECT");
    }


    const SelectSet selectedSet = parseSelectSetQuantifier();
    const std::string tableReference = parseTableReference();
    const SOSQLExpression tableExpression = parseTableExpression();

    // parse end here, check for EOF and semicolon
    // parse where statement here.
    return { selectedSet, tableReference, tableExpression };
}


SOSQLExpression Parser::parseTableExpression() {


    if (!MATCH<WHEREToken>(m_lexer.peek())) {

        return std::make_shared<NULLExpression>();
    }

    m_lexer.nextToken(); // this is the WHERE token;
    return parseORExpression();
}


SOSQLExpression Parser::parseORExpression() {

    SOSQLExpression expression = parseANDExpression();
    while (MATCH<ORToken>(m_lexer.peek())) {

        m_lexer.nextToken(); // scan past or token
        SOSQLExpression right = parseANDExpression();
        expression = std::make_shared<ORExpression>(ORExpression{expression, right});
    }


    return expression;
}


SOSQLExpression Parser::parseANDExpression() {

    SOSQLExpression expression = parseBooleanFactor();
    while (MATCH<ANDToken>(m_lexer.peek())) {

        m_lexer.nextToken(); // scan past and token
        SOSQLExpression right = parseBooleanFactor();
        expression = std::make_shared<ANDExpression>(ANDExpression{expression, right});
    }


    return expression;
}


SOSQLExpression Parser::parseBooleanFactor() {

    if(MATCH<NOTToken>(m_lexer.peek())) {
        m_lexer.nextToken();
        return std::make_shared<NOTExpression>(NOTExpression{parseBooleanExpression()});
    }

    return parseBooleanExpression();
}


SOSQLExpression Parser::parseBooleanExpression() {

    const Token lhs = m_lexer.nextToken();
    if (!MATCH<ColumnToken, QueryResultToken, NumericToken>(lhs)) {

        throw std::invalid_argument("Invalid token type specified in expression: " + getTokenString(lhs));
    }

    return std::visit(overloaded {
            [=] (ComparisonToken) { return parseComparisonExpression(lhs); },
            [=] (ISToken)             { return parseISExpression(lhs); },
            [=] (BETWEENToken) {  return parseBETWEENExpression(lhs); }, 
            [=] (auto const t) -> SOSQLExpression {
                const std::string exceptionString = "Invalid operator token in expression: " + getTokenString(t);
                throw std::invalid_argument(exceptionString); 
            } },
        m_lexer.peek());
}


SOSQLExpression Parser::parseComparisonExpression(const Token lhs) {

    const ComparisonToken comp = std::get<ComparisonToken>(m_lexer.nextToken());
    const Token rhs = m_lexer.nextToken();
    if (!MATCH<ColumnToken, QueryResultToken, NumericToken>(rhs)) {

        throw std::invalid_argument("Invalid token type specified in expression: " + getTokenString(rhs));
    }

    return std::make_shared<ComparisonExpression>(ComparisonExpression{comp.m_opType, lhs, rhs});
}


SOSQLExpression Parser::parseISExpression(const Token lhs) {

    Token rhs = m_lexer.nextToken();
    ComparisonToken::OpType op = ComparisonToken::OP_EQ;
    if (MATCH<NOTToken>(rhs)) {
        op = ComparisonToken::OP_NE;
        rhs = m_lexer.nextToken();
    }

    if (!MATCH<ColumnToken, QueryResultToken, NumericToken>(rhs)) {

        throw std::invalid_argument("Invalid token type specified in expression: " + getTokenString(rhs));
    }

    return std::make_shared<ComparisonExpression>(ComparisonExpression{op, lhs, rhs});
}


SOSQLExpression Parser::parseBETWEENExpression(const Token lhs) {

    m_lexer.nextToken();
    if (!MATCH<NumericToken>(m_lexer.peek())) {
        throw std::invalid_argument("Only numeric tokens can be specified in the BETWEEN clause");
    }

    const NumericToken lowerBound = std::get<NumericToken>(m_lexer.nextToken());
    if (!MATCH<ANDToken>(m_lexer.nextToken())) {
        throw std::invalid_argument("AND keyword missing from BETWEEN clause");
    }

    else if (!MATCH<NumericToken>(m_lexer.peek())) {
        throw std::invalid_argument("Second numeric token missing from BETWEEN clause");
    }

    const NumericToken upperBound = std::get<NumericToken>(m_lexer.nextToken());
    return std::make_shared<BETWEENExpression>(BETWEENExpression{lhs, lowerBound.m_value, upperBound.m_value});
}

SelectSet Parser::parseSelectSetQuantifier() {

    return std::visit(overloaded {
            [=] (ColumnToken) { 
                return parseSelectList(); 
            },
            [=] (PunctuationToken<'*'>) -> SelectSet { 
                m_lexer.nextToken(); 
                return { ColumnToken::PORT, ColumnToken::TCP, ColumnToken::UDP }; 
            },
            [=] (auto t) -> SelectSet {
                const std::string exceptionString = "Invalid token in select list: " + getTokenString(t);
                throw std::invalid_argument(exceptionString); 
            } },
        m_lexer.peek());

};

std::vector<ColumnToken::Column> Parser::parseSelectList() {

    SelectSet selectedSet = { };
    for (bool moreColumns = true; moreColumns;) {
        std::visit(overloaded { 
                [&] (ColumnToken c) { 
                    if (selectedSet.end() != std::find(selectedSet.begin(), selectedSet.end(), c.m_column)) {
                        std::string exceptionString = "Invalid token specified in select list: " + getTokenString(c);
                        throw std::invalid_argument(exceptionString);
                    }

                    selectedSet.push_back(c.m_column);
                    m_lexer.nextToken();
                    moreColumns = MATCH<PunctuationToken<','>>(m_lexer.peek());
                },

                [&] (PunctuationToken<','>) {
                    m_lexer.nextToken();
                },
                [=] (const auto t) -> void { 
                    std::string exceptionString = "Invalid token specified in select list: " + getTokenString(t);
                    throw std::invalid_argument(exceptionString);
                } },
           
            m_lexer.peek());
    }

    return selectedSet;
}


std::string Parser::parseTableReference() {

    const Token t = m_lexer.nextToken();
    if (!MATCH<FROMToken>(t)) {
        const std::string exceptionString = "FROM token not following column list, invalid token specified: " + getTokenString(t);
        throw std::invalid_argument(exceptionString);
    }

    return std::visit(overloaded {
            [=] (UserToken u)  { return u.m_UserToken; },
            [=] (auto t) -> std::string { 
                throw std::invalid_argument("Invalid token following FROM keyword: " + getTokenString(t)); } 
            }, 
        m_lexer.nextToken());
}
