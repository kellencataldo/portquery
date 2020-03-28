#include <exception>
#include <cctype>

#include "Lexer.h"


Token Lexer::nextToken() {

    // This is an optimization for the scenario that the next token has been scanned and identified
    // but the input hasn't been advanced. This occurs when a token is "peeked"

    // First, check to see if we have already parsed the next token, just not advanced the input
    if (std::nullopt != m_peekToken) {
        // move the value that was in the peek token to the return token
        Token outToken(std::move(m_peekToken.value()));
        // Reset the peek token to an empty state, will be set again next time peek() is called
        m_peekToken.reset();
        // Return the token to the user
        return outToken;
    }

    // The peek token was empty, parse the next token the usual way.
    
    return scanNextToken();
}


Token Lexer::scanNextToken() {

    // If m_currentChar is set to the EOF has already been returned, 
    // throw an error in the event that scan is called again
    if (m_queryString.end() == m_currentChar) {
        // out of range error is used here, but logic error would serve just as well
        throw std::out_of_range("Cannot scan past EOF token");
    }

    // advance the character (it was stopped at the end of the last token)
    // and scan past any initial whitespace 
    // @TODO: maybe make this cooler or something?
    while(m_queryString.end() != ++m_currentChar && std::isspace(*m_currentChar)) { m_currentChar++; }

    // We have reached the end of our string, return the EOF token
    if (m_queryString.end() == m_currentChar) { return EOFToken{}; }

    // and now... the real fun begins.
    // Set the token start to wherever the whitespace ended
    m_tokenStart = m_currentChar;

    switch(*m_currentChar) {
        case '*': return PunctuationToken<'*'>{};
        case '(': return PunctuationToken<'('>{};
        case ')': return PunctuationToken<')'>{};
        case ',': return PunctuationToken<','>{};
        case ';': return PunctuationToken<';'>{}; // return EOF here?
        default: return EOFToken{};
    }

    return EOFToken{};
}