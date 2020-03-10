#pragma once
#include "RegularExpressions.h"
RegularExp::CharacterSelect reg_seperator_token(" \t");
RegularExp::StarAtleastOne reg_accepted_seperator(reg_seperator_token);
RegularExp::Star reg_accepted_seperator_star(reg_seperator_token);

RegularExp::CharacterSelect reg_pointer("*&");
RegularExp::Star reg_pointer_star(reg_pointer);

RegularExp::CharacterSelect reg_name_special_tokens("_");

RegularExp::OR reg_name_part1({&RegularExp::g_letter, &reg_name_special_tokens});
RegularExp::OR reg_name_part2({&reg_name_part1, &RegularExp::g_numberPositive});
RegularExp::Star reg_name_part2_star(RegularExp::g_numberPositive);

RegularExp::AND reg_name_whole({&reg_name_part1, &reg_name_part2_star});

int DataType(char*& s, char*& e) {
	const char* start = s;

	if (reg_name_whole.Check(s, e) > 0 && reg_pointer_star.Check(s, e) > 0 && reg_accepted_seperator.Check(s, e) > 0) {
		return s - start;
	}

	s = (char*)start;
	return -1;
}

int Function(char*& s, char*& e) {
	const char* start = s;
	if (DataType(s, e) >= 0) {
		//if (reg_name_whole()) {
		//
		//}
	}
}

int CppStart(const char*& codestart, char*& codeEnd) {
	char* start = (char*)codestart;



}