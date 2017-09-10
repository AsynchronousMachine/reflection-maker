LDLIBS          := -lclangTooling -lclangFrontendTool -lclangFrontend -lclangDriver -lclangSerialization -lclangCodeGen \
                   -lclangParse -lclangSema -lclangStaticAnalyzerFrontend -lclangStaticAnalyzerCheckers -lclangStaticAnalyzerCore -lclangAnalysis \
                   -lclangARCMigrate -lclangRewrite -lclangRewriteFrontend -lclangEdit -lclangASTMatchers -lclangAST -lclangLex -lclangBasic -lclang -lstdc++fs
TARGET_PLATFORM ?= linux
TARGET          = map_maker
STRIP           ?= strip -s
CPPFLAGS        += -std=c++1z -Ofast -DTARGET_PLATFORM=$(TARGET_PLATFORM) -I/usr/lib/llvm-3.9/include -L/usr/lib/llvm-3.9/lib -lLLVM-3.9
CXX             = clang++-3.9

all: $(TARGET)

$(TARGET): *.cpp 
	$(CXX) $(LDFLAGS) $(CPPFLAGS) -o $(TARGET) *.cpp $(LDLIBS)
	$(STRIP) $@

clean:
	-rm -fr $(TARGET) *.dwo

.PHONY: all clean