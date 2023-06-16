LDLIBS          := -lclangTooling -lclangFrontendTool -lclangFrontend -lclangDriver -lclangSerialization -lclangCodeGen \
                   -lclangParse -lclangSema -lclangStaticAnalyzerFrontend -lclangStaticAnalyzerCheckers -lclangStaticAnalyzerCore -lclangAnalysis \
                   -lclangARCMigrate -lclangRewrite -lclangRewriteFrontend -lclangEdit -lclangASTMatchers -lclangAST -lclangLex -lclangBasic -lclang -lstdc++fs

CLVERSION       ?= 10

TARGET_PLATFORM ?= linux
TARGET          := map_maker
STRIP           ?= strip -s
ECHO            ?= echo
CPPFLAGS        += -std=c++17 -Ofast -DTARGET_PLATFORM=$(TARGET_PLATFORM) -I/usr/lib/llvm-$(CLVERSION)/include -L/usr/lib/llvm-$(CLVERSION)/lib -lLLVM-$(CLVERSION)
CXX             = clang++-$(CLVERSION)

.PHONY: help all clean

help:
	@$(ECHO) "Available targets are:"
	@$(ECHO) "make clean"
	@$(ECHO) "make all, if you want to compile with clang 10"
	@$(ECHO) "make CLVERSION:=3.9 all, if you want to compile with clang 3.9"

all: $(TARGET)
	ln -s compile_commands-$(CLVERSION).json compile_commands.json

$(TARGET): *.cpp 
	$(CXX) $(LDFLAGS) $(CPPFLAGS) -o $(TARGET) *.cpp $(LDLIBS)
	$(STRIP) $@

clean:
	-rm -fr $(TARGET) *.dwo compile_commands.json map_maker
