// g++ -std=c++11 map_maker.cpp -o map_maker -lclangAST -lclangASTMatchers -lclangFrontend -lclangBasic -lLLVMSupport -lclangTooling
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <llvm/Support/CommandLine.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <iostream>

// see AST matcher reference for available macros at http://clang.llvm.org/docs/LibASTMatchersReference.html
using namespace clang::ast_matchers;

auto DOFieldMatcher = fieldDecl(hasType(recordDecl(hasName("DataObject")))).bind("DO");
auto LinkFieldMatcher = fieldDecl(hasType(recordDecl(hasName("Link")))).bind("link");
auto ModuleClassMatcher = cxxRecordDecl(hasMethod(hasName("deserialize")), hasMethod(hasName("serialize")), hasMethod(hasName("getName")), has(DOFieldMatcher)).bind("moduleClass");
auto ModuleInstanceMatcher = varDecl(hasType(ModuleClassMatcher)).bind("moduleInstance");

std::unordered_multimap<std::string, std::string> DOs; // key: class, value: DO
std::unordered_multimap<std::string, std::string> Links; // key: class, value: link
std::unordered_map<std::string, std::string> ModuleInstances; // key: variable, value: class

class MapMaker : public clang::ast_matchers::MatchFinder::MatchCallback
{
public:
    void run(const clang::ast_matchers::MatchFinder::MatchResult &Result) override
    {
        if (const clang::RecordDecl *rd = Result.Nodes.getNodeAs<clang::RecordDecl>("moduleClass"))
        {
            std::string moduleClass = rd->getNameAsString();
            if (const clang::VarDecl *vd = Result.Nodes.getNodeAs<clang::VarDecl>("moduleInstance"))
                ModuleInstances.emplace(vd->getNameAsString(), moduleClass);
        }
        else if (const clang::FieldDecl *fd = Result.Nodes.getNodeAs<clang::FieldDecl>("DO"))
            DOs.emplace(fd->getParent()->getNameAsString(), fd->getNameAsString());
        else if (const clang::FieldDecl *fd = Result.Nodes.getNodeAs<clang::FieldDecl>("link"))
            Links.emplace(fd->getParent()->getNameAsString(), fd->getNameAsString());
    }
};

static llvm::cl::OptionCategory MapMakerCategory("MapMaker options");
static llvm::cl::extrahelp CommonHelp(clang::tooling::CommonOptionsParser::HelpMessage);

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

    std::cout << "using dataobject_map = std::unordered_map<std::string, boost::any>;\n";
    std::cout << "dataobject_map dos{\n";
    for (auto &ModuleInstance : ModuleInstances)
    {
        auto range = DOs.equal_range(ModuleInstance.second);
        for (auto it = range.first; it != range.second; ++it)
        {
            std::cout << "{" + ModuleInstance.first + ".getName() + \".\" + " + ModuleInstance.first + "." + it->second + ".getName(), &" + ModuleInstance.first + "." + it->second + "},\n";
        }
    }
    std::cout << "};\n";

    std::cout << "using registerlink_map = std::unordered_map<std::string, std::function<void(std::string, boost::any, boost::any)>>;\n";
    std::cout << "registerlink_map set_links{\n";
    for (auto &ModuleInstance : ModuleInstances)
    {
        auto range = Links.equal_range(ModuleInstance.second);
        for (auto it = range.first; it != range.second; ++it)
        {
            std::cout << "{" + ModuleInstance.first + ".getName() + \".\" + " + ModuleInstance.first + "." + it->second + ".getName(), [&" + ModuleInstance.first + "](std::string name, boost::any a1, boost::any a2){auto l = std::mem_fn(&" + ModuleInstance.second + "::" + it->second + "); l(" + ModuleInstance.first + ").set(name, a1, a2);}},\n";
        }
    }
    std::cout << "};\n";

    std::cout << "using unregisterlink_map = std::unordered_map<std::string, std::function<void(std::string, boost::any)>>;\n";
    std::cout << "unregisterlink_map clear_links{\n";
    for (auto &ModuleInstance : ModuleInstances)
    {
        auto range = Links.equal_range(ModuleInstance.second);
        for (auto it = range.first; it != range.second; ++it)
        {
            std::cout << "{" + ModuleInstance.first + ".getName() + \".\" + " + ModuleInstance.first + "." + it->second + ".getName(), [&" + ModuleInstance.first + "](std::string name, boost::any a){auto l = std::mem_fn(&" + ModuleInstance.second + "::" + it->second + "); l(" + ModuleInstance.first + ").clear(name, a);}},\n";
        }
    }
    std::cout << "};\n";

    std::cout << "using deserialize_map = std::unordered_map<std::string, std::function<void(std::string)>>;\n";
    std::cout << "const deserialize_map des_modules{\n";
    for (auto &ModuleInstance : ModuleInstances)
        std::cout << "{" + ModuleInstance.first + ".getName(), [&" + ModuleInstance.first + "](std::string js){std::mem_fn(&" + ModuleInstance.second + "::deserialize)(" + ModuleInstance.first + ", js);}},\n";
    std::cout << "};\n";

    std::cout << "using serialize_map = std::unordered_map<std::string, std::function<std::string()>>;\n";
    std::cout << "const serialize_map s_modules{\n";
    for (auto &ModuleInstance : ModuleInstances)
        std::cout << "{" + ModuleInstance.first + ".getName(), [&" + ModuleInstance.first + "](){return std::mem_fn(&" + ModuleInstance.second + "::serialize)(" + ModuleInstance.first + ");}},\n";
    std::cout << "};\n";

    return Result;
}
