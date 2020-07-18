#pragma once
#include "RegularExpressions.h"
#include <vector>
#include <string>

////////////////////////////////
//    Code Specific    //
////////////////////////////////

namespace CodeParse {
	enum ParseTreeNodeType {
		ParseTreeNodeType_Root = 0,
		ParseTreeNodeType_Function,
		ParseTreeNodeType_Variable,
		ParseTreeNodeType_DataType,
		ParseTreeNodeType_Name,
		ParseTreeNodeType_Scope,
	};

	static const char* ParseTreeNodeType_string[] = {
		"Root"    ,
		"Function",
		"Variable",
		"DataType",
		"Name"    ,
		"Scope"   ,
	};

	struct ParseTreeNode {
		~ParseTreeNode() {
			for (auto& e : subnodes) {
				delete e;
			}
		}
		ParseTreeNodeType type;
		std::string value;
		std::vector<ParseTreeNode*> subnodes;
	};

	class ParseExpression : virtual public RegularExp::Expression {
	public:
		virtual int Check(char*& current, ParseTreeNode* parrentNode) const = 0;
	private:
		virtual int Check(char*& current, void* parrentNode) const override;
	};

	class AND : public ParseExpression {
	public:
		AND(const std::initializer_list<const ParseExpression*> args);
		~AND() {};

		virtual int Check(char*& current, ParseTreeNode* parrentNode) const override;
	private:
		const std::vector<const ParseExpression*> m_args;
	};
	/////////////////////////////

	class Name : public ParseExpression {
	public:
		Name() {};
		~Name() {};
		// Inherited via Expression
		virtual int Check(char*& current, ParseTreeNode* parrentNode) const override;
	};
	const Name pars_name;

	class DataType : public ParseExpression {
	public:
		DataType() {};
		~DataType() {};
		// Inherited via Expression
		virtual int Check(char*& current, ParseTreeNode* parrentNode) const override;
	};
	const DataType pars_dataType;

	class Parameter : public ParseExpression {
	public:
		Parameter() {};
		~Parameter() {};
		// Inherited via Expression
		virtual int Check(char*& current, ParseTreeNode* parrentNode) const override;
	};
	const Parameter pars_param;

	class Function : public ParseExpression {
	public:
		Function() {};
		~Function() {};
		// Inherited via Expression
		virtual int Check(char*& current, ParseTreeNode* parrentNode) const override;
	};
	const CodeParse::Function pars_function;

	class Scope : public ParseExpression {
	public:
		Scope();
		~Scope() {};
		// Inherited via Expression
		virtual int Check(char*& current, ParseTreeNode* parrentNode) const override;
	};
	const CodeParse::Scope pars_scope;

	class CodeCpp : public ParseExpression {
	public:
		CodeCpp() {};
		~CodeCpp() {};
		// Inherited via Expression
		virtual int Check(char*& current, ParseTreeNode* parrentNode) const override;
	};
	const CodeParse::CodeCpp pars_code_cpp;

	/*String*/
	const RegularExp::CharacterSelect_Set reg_name_special_tokens("_");
	const RegularExp::OR   reg_name_part1({ &RegularExp::CharacterCollections::g_letter, &reg_name_special_tokens });
	const RegularExp::OR   reg_name_part2({ &reg_name_part1, &RegularExp::Numbers::g_numberPositive });
	const RegularExp::Star reg_name_part2_star(reg_name_part2);
	const RegularExp::AND  reg_string({ &reg_name_part1, &reg_name_part2_star });

	/*Namespace*/
	const RegularExp::CharacterSelect_Set reg_namespace_special_tokens(":");
	const RegularExp::Star_Min_Max    reg_namespace_seperator(reg_namespace_special_tokens, 2, 2);
	const RegularExp::Star_Min_Max    reg_namespace_seperator_onlyOnce(reg_namespace_seperator, 0, 1);

	/*Variable / Dataype Name*/
	const RegularExp::AND  reg_variableName_allowNameSpace({ &reg_namespace_seperator_onlyOnce, &reg_name_part1, &reg_name_part2_star });
	const RegularExp::StarAtleastOne  reg_variableName_allowNameSpace_AtleastOne(reg_variableName_allowNameSpace);

	/*Pointers*/
	const RegularExp::CharacterSelect_Set reg_pointer("*&");
	const RegularExp::Star reg_pointer_star(reg_pointer);

	/*Code Block / Scope*/
	const RegularExp::CharacterSelect_Set reg_scope_start("{");
	const RegularExp::CharacterSelect_Set reg_scope_end("}");
	const RegularExp::AnyExept_Set        reg_anyExept_scope_end("}");

	const RegularExp::OR reg_scope_content({ &pars_scope, &reg_anyExept_scope_end });
	const RegularExp::Star reg_scope_content_star(reg_scope_content);

	/*File Paths*/
	const RegularExp::CharacterSelect_Set reg_path_seperator_startEnd("\\/");
	const RegularExp::StarAtleastOne  reg_path_seperator_atleast_one(reg_path_seperator_startEnd);
	const RegularExp::AND             reg_fileExtension({ &RegularExp::Special::g_dot, &reg_string });
	const RegularExp::Star_Min_Max    reg_specialFolderName(RegularExp::Special::g_dot, 2, 2);
	const RegularExp::OR   reg_FolderName({ &reg_string, &reg_specialFolderName });
	const RegularExp::AND  reg_FolderPath({ &reg_FolderName, &reg_path_seperator_atleast_one });
	const RegularExp::Star reg_FolderName_star(reg_FolderPath);
	const RegularExp::AND  reg_full_Path({ &reg_FolderName_star, &reg_string, &reg_fileExtension });

	/*Includes*/
	const RegularExp::Term reg_includeTerm("#include");
	const RegularExp::CharacterSelect_Set reg_inc1_start("<");
	const RegularExp::CharacterSelect_Set reg_inc1_end(">");
	const RegularExp::CharacterSelect_Set reg_inc2_startEnd("\"");
	const RegularExp::AND reg_inc1({ &reg_inc1_start, &reg_string, &reg_inc1_end });
	const RegularExp::AND reg_inc2({ &reg_inc2_startEnd, &reg_full_Path, &reg_inc2_startEnd });
	const RegularExp::OR  reg_include_select({ &reg_inc1, &reg_inc2 });
	const RegularExp::AND reg_include({ &reg_includeTerm, &RegularExp::Special::g_seperator_atleastOne, &reg_include_select });

	/*Functions*/
	const RegularExp::CharacterSelect_Set reg_function_start("(");
	const RegularExp::CharacterSelect_Set reg_function_end(")");
	const RegularExp::CharacterSelect_Set reg_func_par_var_sep(",");
	const RegularExp::AND  reg_commaSeperated({ &RegularExp::Special::g_seperator_star, &reg_func_par_var_sep, &RegularExp::Special::g_seperator_star });
	const RegularExp::AND  reg_func_params_var_sep({ &reg_commaSeperated, &pars_param });
	const RegularExp::Star reg_func_params_vars_sep_star(reg_func_params_var_sep);
	const RegularExp::AND  reg_func_params_vars_all({ &pars_param, &reg_func_params_vars_sep_star });
	const RegularExp::Star_Min_Max reg_func_params_vars_any(reg_func_params_vars_all, 0, 1);
	const RegularExp::AND  reg_func_params({ &reg_function_start, &RegularExp::Special::g_seperator_star, &reg_func_params_vars_any, &RegularExp::Special::g_seperator_star, &reg_function_end });

	//Code
	const RegularExp::OR reg_code_cpp_Start({ &RegularExp::Special::g_EOL_token, &pars_function, &reg_include, &RegularExp::CharacterCollections::g_any });
	const RegularExp::Star reg_code_cpp_Start_Star(reg_code_cpp_Start);
}

