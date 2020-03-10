#include "stdafx.h"
#include "RegularExpressions.h"
#include "CodeParser.h"

RegularExp::Any::Any() : RegularExp::AnyExept(""){}

RegularExp::Star_Min_Max::Star_Min_Max(const Expression& exp, unsigned int min, unsigned int max) : m_exp(exp), m_min(min), m_max(max) {}

int RegularExp::Star_Min_Max::Check(char*& current, void* tree) const {
	const char* start = current;
	unsigned int found = 0;
	while (current[0] != '\0' && found < m_max && m_exp.Check(current, tree) > 0) {
		found++;
	}
	if (found >= m_min) {
		return current - start;
	} else {
		current = (char*)start;
		return -1;
	}
}

RegularExp::Term::Term(const char* term) : m_term(term) {}

int RegularExp::Term::Check(char*& current, void* tree) const {
	const char* start = current;
	int i = 0;
	while (m_term[i] != '\0') {
		if (m_term[i] != (current++)[0]) {
			current = (char*)start;
			return -1;
		}
		i++;
	}

	return current - start;
}

RegularExp::Star::Star(const Expression& exp) : Star_Min_Max(exp, 0) {}

RegularExp::StarAtleastOne::StarAtleastOne(const Expression& exp) : Star_Min_Max(exp, 1) {}

RegularExp::OR::OR(const std::initializer_list<const Expression*> args) : m_args(args) {}

int RegularExp::OR::Check(char*& current, void* tree) const {
	const char* start = current;

	for (auto& e : m_args) {
		if (e->Check(current, tree) >= 0) {
			return current - start;
		}
	}

	return -1;
}

RegularExp::AND::AND(const std::initializer_list<const Expression*> args) : m_args(args) {}

int CodeParse::AND::Check(char*& current, ParseTreeNode* node) const
{
	const char* start = current;

	for (auto& e : m_args) {
		if (e->Check(current, node) < 0) {
			current = (char*)start;
			return -1;
		}
	}

	return current - start;
}

int RegularExp::AND::Check(char*& current, void* tree) const {
	const char* start = current;

	for (auto& e : m_args) {
		if (e->Check(current, tree) < 0) {
			current = (char*)start;
			return -1;
		}
	}

	return current - start;
}

RegularExp::CharacterSelect::CharacterSelect(const char* characterSet) : m_characterSet(characterSet) {}

int RegularExp::CharacterSelect::Check(char*& current, void* tree) const {
	int i = 0;
	while (m_characterSet[i] != '\0') {
		if (current[0] == m_characterSet[i++]) {
			current++;
			return 1;
		}
	}

	return -1;
}

RegularExp::AnyExept::AnyExept(const char* characterSet) : CharacterSelect(characterSet){}

int RegularExp::AnyExept::Check(char*& current, void* tree) const
{
	const char* start = current;
	if (CharacterSelect::Check(current, tree) < 0) {
		current++;
		return 1;
	}
	current = (char*)start;
	return -1;
}
