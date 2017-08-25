LDLIBS          := 	-lclangTooling -lclangFrontendTool -lclangFrontend -lclangDriver -lclangSerialization -lclangCodeGen \
					-lclangParse -lclangSema -lclangStaticAnalyzerFrontend -lclangStaticAnalyzerCheckers -lclangStaticAnalyzerCore -lclangAnalysis \
					-lclangARCMigrate -lclangRewrite -lclangRewriteFrontend -lclangEdit -lclangASTMatchers -lclangAST -lclangLex -lclangBasic -lclang -lstdc++fs
TARGET_PLATFORM ?= linux
TARGET          := map_maker
STRIP           ?= strip
CPPFLAGS        += -std=c++11 -DTARGET_PLATFORM=$(TARGET_PLATFORM) `llvm-config-3.9 --cxxflags --ldflags --libs`

all: $(TARGET)

$(TARGET): *.cpp 
	$(CXX) $(LDFLAGS) $(CPPFLAGS) -o $(TARGET) *.cpp $(LDLIBS)

clean:
	-rm -fr $(TARGET) *.dwo

.PHONY: all install clean docs