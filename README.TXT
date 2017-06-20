Verwendung von map_maker nach dem Bau mit make:

Jedes Mal: Das submodul asm von github auschecken bzw. aktualisieren.

Falls notwendig Änderung in "compile_commands.json" durchführen.
- directory ist der Pfad, in den der map-maker ausgecheckt wurde.
- evtl. compilerkommandos für asm anpassen, Libraries, Includes etc.

Der Inhalt der Datei muss der JSON Compilation Database Format Specification entsprechen (siehe http://clang.llvm.org/docs/JSONCompilationDatabase.html).

Beispiel:

[
  {
    'directory': '/home/Workspace/map-maker',
    'file': 'asm/src/modules/global_modules.cpp',	
    
    'command': 'clang++ asm/src/modules/global_modules.cpp -o global_modules.obj -std=c++14 -DBOOST_HAS_THREADS -I/usr/lib/llvm-3.9/lib/clang/3.9.0/include'
  }
]

In 'directory' ist der absolute Pfad einzutragen, in der die unter 'file' angegebene Datei liegt.
map_maker kompiliert die Datei mit dem clang Compiler.
'command' ist das benötigte Konsolenkommando zum Kompilieren. Die Angaben müssen gemäß den Clang-Regeln erfolgen.
Es können auch mehrere Kommandos definiert werden

Der Aufruf in der Konsole lautet dann (map_maker benötigt als einzigen Startparameter 'file' als Referenz):
./map_maker asm/src/modules/global_modules.cpp
Dieser Befehl ist bereits in runmaker.sh hinterlegt, so dass nur noch dieses Skript aufgerufen werden muss.