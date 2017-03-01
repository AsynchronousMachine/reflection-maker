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
auto LinkFieldMatcher = fieldDecl(isPublic(), hasType(recordDecl(hasName("Link")))).bind("link");
auto ModuleClassMatcher = cxxRecordDecl(anyOf(has(DOFieldMatcher), has(LinkFieldMatcher))).bind("moduleClass");
auto ModuleInstanceMatcher = varDecl(hasType(ModuleClassMatcher)).bind("moduleInstance");

//map should be sorted for print_modules output
std::multimap<std::string, std::pair<std::string, std::string>> DOs; // key: module class name, value: DO as pair of instance name and datatype
std::multimap<std::string, std::pair<std::string, std::string>> Links; // key: module class name, value: link as pair of instance name and datatype
std::map<std::string, std::string> ModuleInstances; // key: variable, value: module class name

void replaceAll(std::string &s, const std::string &search, const std::string &replace)
{
	for (size_t pos = 0; ; pos += replace.length()) {
		// Locate the substring to replace
		pos = s.find(search, pos);
		if (pos == std::string::npos) break;
		// Replace by erasing and inserting
		s.erase(pos, search.length());
		s.insert(pos, replace);
	}
};

class MapMaker : public clang::ast_matchers::MatchFinder::MatchCallback
{
public:
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
			std::string type = fd->getType().getAsString();
			if (type.find("_Bool") > 0)
			{
				replaceAll(type, "_Bool", "bool");
			}
			DOs.emplace(fd->getParent()->getNameAsString(), make_pair(fd->getNameAsString(), type));
		}
		else if (const clang::FieldDecl *fd = Result.Nodes.getNodeAs<clang::FieldDecl>("link")) {
			Links.emplace(fd->getParent()->getNameAsString(), make_pair(fd->getNameAsString(), fd->getType().getAsString()));
		}
	}
};

static llvm::cl::OptionCategory MapMakerCategory("MapMaker options");
//static llvm::cl::extrahelp CommonHelp(clang::tooling::CommonOptionsParser::HelpMessage);

int main(int argc, const char **argv)
{
	clang::tooling::CommonOptionsParser OptionsParser(argc, argv, MapMakerCategory);
	clang::tooling::ClangTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());

	MapMaker Maker;
	clang::ast_matchers::MatchFinder Finder;
	Finder.addMatcher(DOFieldMatcher, &Maker);
	Finder.addMatcher(LinkFieldMatcher, &Maker);
	Finder.addMatcher(ModuleInstanceMatcher, &Maker);

	int Result = Tool.run(clang::tooling::newFrontendActionFactory(&Finder).get());

	std::fstream fs;

	fs.open("global_reflection.h", std::fstream::out);

	fs << "#pragma once" << std::endl
		<< "#include <boost/variant.hpp>" << std::endl
		<< "#include \"asm.hpp\"" << std::endl
		<< std::endl
		<< "struct EmptyDataobject : boost::blank" << std::endl
		<< "{" << std::endl
		<< "    const std::string serialize(rapidjson::Value& value, rapidjson::Document::AllocatorType& allocator) { return \"!!!Will never be used!!!\"; };" << std::endl
		<< "    const void deserialize(rapidjson::Value& value) {};" << std::endl
		<< "    template <class Visitor>" << std::endl
		<< "    const int get(Visitor visitor) { return 0; }; // !!!Will never be used!!!" << std::endl
		<< "};" << std::endl
		<< std::endl
		<< "using dataobjects = boost::variant<EmptyDataobject&";
	//print all Dataobject types
	std::set<std::string> myset;

	for (const auto value : DOs)
	{
		myset.insert(value.second.second);
	}
	for (const std::string type : myset)
	{
		fs << ", " << type << "&";
	}
	fs << ">;" << std::endl
		<< "using module_name_map = std::unordered_map<void*, std::string>;" << std::endl
		<< "using name_module_map = std::unordered_map<std::string, boost::any>;" << std::endl
		<< "using dataobject_name_map = std::unordered_map<void*, std::string>;" << std::endl
		<< "using name_dataobject_map = std::unordered_map<std::string, dataobjects>;" << std::endl
		<< "using registerlink_map = std::unordered_map<std::string, std::function<void(std::string, boost::any, boost::any)>>;" << std::endl
		<< "using unregisterlink_map = std::unordered_map<std::string, std::function<void(std::string, boost::any)>>;" << std::endl
		<< "using print_module_map = std::map<std::string, std::string>;" << std::endl
		<< std::endl
		<< "extern const module_name_map module_names;" << std::endl
		<< "extern const name_module_map modules;" << std::endl
		<< "extern const dataobject_name_map do_names;" << std::endl
		<< "extern const registerlink_map set_links;" << std::endl
		<< "extern const unregisterlink_map clear_links;" << std::endl
		<< "extern const print_module_map print_modules;" << std::endl
		<< "extern const name_dataobject_map dataobject_map;";

	fs.close();

	fs.open("global_reflection.cpp", std::fstream::out);

	fs << "#include \"global_reflection.h\"" << std::endl << std::endl;
	fs << "const name_module_map modules" << std::endl << "{" << std::endl;
	for (auto &ModuleInstance : ModuleInstances)
	{
		fs << "\t{\"" + ModuleInstance.second + "." + ModuleInstance.first + "\", &" + ModuleInstance.first + "}";
		fs << ",";
		fs << std::endl;
	}
	fs << "};" << std::endl << std::endl;

	fs << "// Get out the module name for humans" << std::endl;
	fs << "const module_name_map module_names" << std::endl << "{" << std::endl;
	for (auto &ModuleInstance : ModuleInstances)
	{
		fs << "\t{&" + ModuleInstance.first + ", \"" + ModuleInstance.second + "." + ModuleInstance.first + "\"}";
		fs << ",";
		fs << std::endl;
	}
	fs << "};" << std::endl << std::endl;

	fs << "const name_dataobject_map dataobject_map" << std::endl << "{" << std::endl;
	for (auto &ModuleInstance : ModuleInstances)
	{
		auto range = DOs.equal_range(ModuleInstance.second);
		for (auto it = range.first; it != range.second; ++it)
		{
			fs << "\t{\"" + ModuleInstance.second + "." + ModuleInstance.first + "." + it->second.first + "\", " + ModuleInstance.first + "." + it->second.first + "}," << std::endl;
		}
	}
	fs << "};" << std::endl << std::endl;

	fs << "// Get out the DO name for humans" << std::endl;
	fs << "const dataobject_name_map do_names" << std::endl << "{" << std::endl;
	for (auto &ModuleInstance : ModuleInstances)
	{
		auto range = DOs.equal_range(ModuleInstance.second);
		for (auto it = range.first; it != range.second; ++it)
		{
			fs << "\t{&" + ModuleInstance.first + "." + it->second.first + ", \"" + ModuleInstance.second + "." + ModuleInstance.first + "." + it->second.first + "\"}," << std::endl;
		}
	}
	fs << "};" << std::endl << std::endl;

	fs << "const registerlink_map set_links" << std::endl << "{" << std::endl;
	for (auto &ModuleInstance : ModuleInstances)
	{
		auto range = Links.equal_range(ModuleInstance.second);
		for (auto it = range.first; it != range.second; ++it)
		{
			fs << "\t{\"" + ModuleInstance.second + "." + ModuleInstance.first + "." + it->second.first + "\", [](std::string name, boost::any a1, boost::any a2) {" + ModuleInstance.first + "." + it->second.first + ".set(name, a1, a2); } }," << std::endl;
		}
	}
	fs << "};" << std::endl << std::endl;


	fs << "const unregisterlink_map clear_links" << std::endl << "{" << std::endl;;
	for (auto &ModuleInstance : ModuleInstances)
	{
		auto range = Links.equal_range(ModuleInstance.second);
		for (auto it = range.first; it != range.second; ++it)
		{
			fs << "\t{\"" + ModuleInstance.second + "." + ModuleInstance.first + "." + it->second.first + "\", [](std::string name, boost::any a) {" + ModuleInstance.first + "." + it->second.first + ".clear(name, a); } }," << std::endl;
		}
	}
	fs << "};" << std::endl << std::endl;

	//map for sorted order
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

	fs.close();

	return Result;
}
