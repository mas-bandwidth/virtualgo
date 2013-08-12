#include "AssertException.h"
#include <cstring>

namespace UnitTest {

#ifdef _MSC_VER
#pragma warning(disable: 4786) // 'identifier' : identifier was truncated to 'number' characters
#pragma warning(disable: 4996) // disable security warnings about sprintf etc.
#pragma warning(disable: 4800) // forcing value to bool true or false
#endif

    AssertException::AssertException(char const* description, char const* filename, int lineNumber)
    : m_lineNumber(lineNumber)
{
	using namespace std;

    strcpy(m_description, description);
    strcpy(m_filename, filename);
}

AssertException::~AssertException() throw()
{
}

char const* AssertException::what() const throw()
{
    return m_description;
}

char const* AssertException::Filename() const
{
    return m_filename;
}

int AssertException::LineNumber() const
{
    return m_lineNumber;
}

}
