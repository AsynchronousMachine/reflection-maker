Todo nachdem map_maker mit make gebaut wurde:

map_maker in das Verzeichnis mit der global_instances Datei ablegen.
Außerdem wird in diesem Verzeichnis eine Datei mit dem Namen "compile_commands.json" benötigt.
Der Inhalt der Datei muss der JSON Compilation Database Format Specification entsprechen (siehe http://clang.llvm.org/docs/JSONCompilationDatabase.html).

Beispiel:

[
  {
    'directory': '/home/Workspace/Finalconcept-reflection-maker',
    'file': 'global_instances.h',	
    'command': 'clang++.exe -xc++ global_instances.h -o global_instances.obj -std=c++14  -I/usr/lib/llvm-3.9/lib/clang/3.9.0/include '
  }
]

In 'directory' ist der absolute Pfad einzutragen, in der die unter 'file' angegebene Datei liegt.
map_maker kompiliert die Datei mit dem clang Compiler.
'command' ist das benötigte Konsolenkommando zum Kompilieren. Die Angaben müssen gemäß den Clang-Regeln erfolgen.
Es können auch mehrere Kommandos definiert werden

Der Aufruf in der Konsole lautet dann (map_maker benötigt als einzigen Startparameter 'file' als Referenz):
./map_maker global_instances.h
