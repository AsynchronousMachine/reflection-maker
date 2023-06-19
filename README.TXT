Verwendung von "map_maker" nach dem Bauen mit make:

Jedes Mal: Das Submodul "asm" auschecken oder ggfs. aktualisieren.

Falls notwendig Änderung in "compile_commands-x.y.json" durchführen:
  - "directory" ist der Pfad, in dem der "map-maker" ausgecheckt wurde.
  - evtl. Compiler-Kommandos für "asm" anpassen, Libraries, Includes etc.

Der Inhalt der Datei muss der JSON Compilation Database Format Specification entsprechen (siehe http://clang.llvm.org/docs/JSONCompilationDatabase.html).

Beispiel:

[
  {
    "directory": "/opt/Projects/AsynchronousMachine/reflection-maker",
    "file": "asm/src/modules/global_modules.cpp",    
    "command": "clang++ asm/src/modules/global_modules.cpp -o global_modules.obj -I/usr/lib/llvm-15/lib/clang/15.0.7/include -I/opt/local/include"
  }
]

- In "directory" ist der absolute Pfad einzutragen, in der die unter "file" angegebene Datei liegt.
- "map_maker" kompiliert die Datei mit dem Clang-Compiler.
- "command" ist das benötigte Konsolenkommando zum Kompilieren. Die Angaben müssen gemäß den Clang Aufruf-Regeln erfolgen.

Der Aufruf in der Konsole lautet dann ("map_maker" benötigt als einzigen Startparameter "file" als Referenz):
  - "./map_maker asm/src/modules/global_modules.cpp"

Dieser Befehl ist bereits in "runmaker.sh" hinterlegt, so dass nur noch dieses Skript aufgerufen werden muss.

Wenn alles fehlerfrei funktioniert sind im Verzeichnis "maker" (falls nicht vorhanden, wird es erstellt) die erzeugten Dateien für das Reflection und das Verzeichnis muss dann nach "asm/src" kopiert werden, so dass "asm/src/maker" dort erscheint.

