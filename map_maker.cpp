// g++ -std=c++11 map_maker.cpp -o map_maker -lclangAST -lclangASTMatchers -lclangFrontend -lclangBasic -lLLVMSupport -lclangTooling
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <llvm/Support/CommandLine.h>
#include <set>
#include <vector>
#include <utility>
#include <string>
#include <fstream>

// see AST matcher reference for available macros at http://clang.llvm.org/docs/LibASTMatchersReference.html
using namespace clang::ast_matchers;

auto DOFieldMatcher = fieldDecl(isPublic(), hasType(recordDecl(hasName("DataObject")))).bind("DO");
auto LinkFieldMatcher = fieldDecl(isPublic(), hasType(recordDecl(hasName("LinkObject")))).bind("link");
auto ModuleClassMatcher = cxxRecordDecl(anyOf(has(DOFieldMatcher), has(LinkFieldMatcher))).bind("moduleClass");
auto ModuleInstanceMatcher = varDecl(hasType(ModuleClassMatcher)).bind("moduleInstance");

std::multimap<std::string, std::pair<std::string, std::string>> DOs; // key: module class name, value: DO as pair of instance name and datatype
std::multimap<std::string, std::pair<std::string, std::string>> Links; // key: module class name, value: link as pair of instance name and datatype
//map should be sorted for print_modules output
std::map<std::string, std::string> ModuleInstances; // key: variable, value: module class name
std::set<std::string> DOSet;
std::set<std::string> LinkSet;


class MapMaker : public clang::ast_matchers::MatchFinder::MatchCallback
{
public:
	MapMaker() : printingPolicy(getLangOptions()) {}

	void run(const clang::ast_matchers::MatchFinder::MatchResult &Result) override
	{
		if (const clang::RecordDecl *rd = Result.Nodes.getNodeAs<clang::RecordDecl>("moduleClass"))
		{
			std::string moduleClass = rd->getNameAsString();
			if (const clang::VarDecl *vd = Result.Nodes.getNodeAs<clang::VarDecl>("moduleInstance")) {
				ModuleInstances.emplace(vd->getNameAsString(), moduleClass);
			}
		}
		else if (const clang::FieldDecl *fd = Result.Nodes.getNodeAs<clang::FieldDecl>("DO")) {
			DOs.emplace(fd->getParent()->getNameAsString(), make_pair(fd->getNameAsString(), fd->getType().getAsString(printingPolicy)));
		}
		else if (const clang::FieldDecl *fd = Result.Nodes.getNodeAs<clang::FieldDecl>("link")) {
			Links.emplace(fd->getParent()->getNameAsString(), make_pair(fd->getNameAsString(), fd->getType().getAsString(printingPolicy)));
		}
	}

private:
	static clang::LangOptions getLangOptions()
	{
		clang::LangOptions langOptions;
		langOptions.Bool = true;
		return langOptions;
	}

	clang::PrintingPolicy printingPolicy;
};

static llvm::cl::OptionCategory MapMakerCategory("MapMaker options");
//static llvm::cl::extrahelp CommonHelp(clang::tooling::CommonOptionsParser::HelpMessage);

int main(int argc, const char **argv)
{
	std::fstream fs;

	 /**
     * create needed files for clang compiling process
     */

	//asm/maker_do.hpp
	fs.open("asm/maker_do.hpp", std::fstream::out);

	fs << "#pragma once" << std::endl << std::endl
		<< "#include <boost/variant.hpp>" << std::endl << std::endl
		<< "#include \"dataobject.hpp\"" << std::endl
		<< "#include \"../datatypes/global_datatypes.hpp\"" << std::endl << std::endl

		<< "namespace Asm {" << std::endl
		<< "\tusing data_variant = boost::variant<" << std::endl
		<< "\t\tEmptyDataobject&" << std::endl
		<< "\t>;" << std::endl
		<< "}" << std::endl;

	fs.close();


	//asm/maker_link.hpp
	fs.open("asm/maker_link.hpp", std::fstream::out);

	fs << "#pragma once" << std::endl << std::endl
		<< "#include <boost/variant.hpp>" << std::endl << std::endl
		<< "#include \"dataobject.hpp\"" << std::endl
		<< "#include \"linkobject.hpp\"" << std::endl
		<< "#include \"../datatypes/global_datatypes.hpp\"" << std::endl << std::endl

		<< "namespace Asm {" << std::endl
		<< "\tusing link_variant = boost::variant<" << std::endl
		<< "\t\tEmptyLinkObject&" << std::endl
		<< "\t>;" << std::endl
		<< "}" << std::endl;

	fs.close();

	/**
     * clang processing
     */
	 
	clang::tooling::CommonOptionsParser OptionsParser(argc, argv, MapMakerCategory);
	clang::tooling::ClangTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());

	MapMaker Maker;
	clang::ast_matchers::MatchFinder Finder;
	Finder.addMatcher(DOFieldMatcher, &Maker);
	Finder.addMatcher(LinkFieldMatcher, &Maker);
	Finder.addMatcher(ModuleInstanceMatcher, &Maker);

	int Result = Tool.run(clang::tooling::newFrontendActionFactory(&Finder).get());


	 /**
     * write maker generated files
     */
	 
	//modules/maker_reflection.hpp
	fs.open("modules/maker_reflection.hpp", std::fstream::out);

	fs << "#pragma once" << std::endl << std::endl
		<< "#include \"../asm/asm.hpp\"" << std::endl
		<< std::endl
		<< "using id_string_map = std::unordered_map<void*, std::string>;" << std::endl
		<< "using name_dataobject_map = std::unordered_map<std::string, Asm::data_variant>;" << std::endl
		<< "using name_link_map = std::unordered_map<std::string, Asm::link_variant>;" << std::endl
		<< "using print_module_map = std::map<std::string, std::string>;" << std::endl
		<< std::endl
		<< "extern const id_string_map module_name;" << std::endl
		<< "extern const id_string_map do_names;" << std::endl
		<< "extern const name_dataobject_map name_dataobjects;" << std::endl
		<< "extern const name_link_map name_links;" << std::endl
		<< "extern const print_module_map print_modules;" << std::endl;

	fs.close();

	//modules/maker_reflection.cpp
	fs.open("modules/maker_reflection.cpp", std::fstream::out);

	fs << "#include \"global_modules.hpp\"" << std::endl;
	fs << "#include \"maker_reflection.hpp\"" << std::endl;


	fs << "// Get out the module name for humans" << std::endl;
	fs << "const id_string_map module_name" << std::endl << "{" << std::endl;
	for (auto &ModuleInstance : ModuleInstances)
	{
		fs << "\t{&" + ModuleInstance.first + ", \"" + ModuleInstance.second + "." + ModuleInstance.first + "\"}";
		fs << ",";
		fs << std::endl;
	}
	fs << "};" << std::endl << std::endl;


	fs << "// Get out the DO name for humans" << std::endl;
	fs << "const id_string_map do_names" << std::endl << "{" << std::endl;
	for (auto &ModuleInstance : ModuleInstances)
	{
		auto range = DOs.equal_range(ModuleInstance.second);
		for (auto it = range.first; it != range.second; ++it)
		{
			fs << "\t{&" + ModuleInstance.first + "." + it->second.first + ", \"" + ModuleInstance.second + "." + ModuleInstance.first + "." + it->second.first + "\"}," << std::endl;
		}
	}
	fs << "};" << std::endl << std::endl;


	//map for sorted order
	fs << "// Get out by name all modules, all their dataobjects with type and their links for humans" << std::endl;
	fs << "const print_module_map print_modules" << std::endl << "{" << std::endl;
	for (auto &ModuleInstance : ModuleInstances)
	{
		fs << "\t{\"" + ModuleInstance.first + "\",\"" + ModuleInstance.second + "::" + ModuleInstance.first + "\\n";


		auto range = DOs.equal_range(ModuleInstance.second);
		for (auto it = range.first; it != range.second; ++it)
		{
			fs << "  |> " << it->second.second << " " << it->second.first + "\\n";
		}
		auto range2 = Links.equal_range(ModuleInstance.second);
		for (auto it = range2.first; it != range2.second; ++it)
		{
			fs << "  |> " << it->second.second << " " << it->second.first + "\\n";
		}

		fs << "\"}," << std::endl;
	}
	fs << "};" << std::endl << std::endl;


	fs << "const name_dataobject_map name_dataobjects" << std::endl << "{" << std::endl;
	for (auto &ModuleInstance : ModuleInstances)
	{
		auto range = DOs.equal_range(ModuleInstance.second);
		for (auto it = range.first; it != range.second; ++it)
		{
			fs << "\t{\"" + ModuleInstance.second + "." + ModuleInstance.first + "." + it->second.first + "\", " + ModuleInstance.first + "." + it->second.first + "}," << std::endl;
			DOSet.insert(it->second.second);
		}
	}
	fs << "};" << std::endl << std::endl;


	fs << "const name_link_map name_links" << std::endl << "{" << std::endl;
	for (auto &ModuleInstance : ModuleInstances)
	{
		auto range = Links.equal_range(ModuleInstance.second);
		for (auto it = range.first; it != range.second; ++it)
		{
			fs << "\t{\"" + ModuleInstance.second + "." + ModuleInstance.first + "." + it->second.first + "\", " + ModuleInstance.first + "." + it->second.first + "}," << std::endl;
			LinkSet.insert(it->second.second);
		}
	}
	fs << "};" << std::endl << std::endl;

	fs.close();


	//asm/maker_do.hpp
	fs.open("asm/maker_do.hpp", std::fstream::out);

	fs << "#pragma once" << std::endl << std::endl
		<< "#include <boost/variant.hpp>" << std::endl << std::endl
		<< "#include \"dataobject.hpp\"" << std::endl
		<< "#include \"../datatypes/global_datatypes.hpp\"" << std::endl << std::endl

		<< "namespace Asm {" << std::endl
		<< "\tusing data_variant = boost::variant<" << std::endl
		<< "\t\tEmptyDataobject&";

	for (const std::string type : DOSet)
	{
		fs << ", " << std::endl << "\t\t" << type << "&";
	}

	fs << std::endl << "\t>;" << std::endl
		<< "}" << std::endl;

	fs.close();


	//asm/maker_link.hpp
	fs.open("asm/maker_link.hpp", std::fstream::out);

	fs << "#pragma once" << std::endl << std::endl
		<< "#include <boost/variant.hpp>" << std::endl << std::endl
		<< "#include \"dataobject.hpp\"" << std::endl
		<< "#include \"linkobject.hpp\"" << std::endl
		<< "#include \"../datatypes/global_datatypes.hpp\"" << std::endl << std::endl

		<< "namespace Asm {" << std::endl
		<< "\tusing link_variant = boost::variant<" << std::endl
		<< "\t\tEmptyLinkObject&";

	for (const std::string type : LinkSet)
	{
		fs << ", " << std::endl << "\t\t" << type << "&";
	}

	fs << std::endl << "\t>;" << std::endl
		<< "}" << std::endl;

	fs.close();

	return Result;
}