============================================
Clang Static Analyzer Frontend Documentation
============================================

.. contents:: Table of Contents
   :depth: 4

Introduction
------------

This document will describe the frontend of the Static Analyzer, basically everything from compiling the analyzer from source, through it's invocation up to the beginning of the analysis. It will touch on topics such as

* How the analyzer is compiled, how tools such as TableGen are used to generate some of the code,
* How to invoke the analyzer,
* How crucial objects of the analyzer are initialized before the actual analysis begins, like

  * The `AnalyzerOptions` class, which entails how the command line options are parsed,
  * The `CheckerManager` class, which entails how the checkers of the analyzer are registered and loaded into it
  * No list is complete without at least a third item.

* How certain errors are handled with regards to backward compatibility


starting from how an entry in the TableGen gets processed during the compilation of the project, how this process begins runtime when the analyzer is invoked, up to the point where the actual analysis begins.

The document will rely on the reader having a basic understanding about what checkers are, have invoked the analyzer at least a few times from the command line. If you also have at least registered a checker in the past up to the point where it shows up in ``clang -cc1 -analyzer-checker-help``, that's a plus, but not a requirement.

Overview
^^^^^^^^

Compilation
***********

The Static Analyzer consists of 3 libraries, ``libStaticAnalyzerCore``, ``libStaticAnalyzerCheckers`` and ``libStaticAnalyzerFrontend``. The checker library depends on core, and frontend depends on both. Before any of them are compiled, TableGen is run on Checkers.td_, according to the rules defined in ClangSACheckersEmitter.cpp_, and generates the file Checkers.inc. By using the preprocessor, this, and other definition files (with the extension ``*.def``) are converted into actual code, such as fields within ``AnalyzerOptions`` and function calls for registering checkers in ``CheckerRegistry``.

Following this, the compilation goes on as usual. The fastest way of obtaining the analyzer for development is by configuring CMake with the following options:

* Use the `Ninja` build system
* Build in `Release` with asserts enabled (Only recommended for slower computers!)
* Build shared libraries
* Only build a single target triple
* Use clang as the C/C++ compiler
* Use gnu gold, or even better, LLD as a linker

An example configuration:

.. code-block:: bash
  cmake \
    -G "Ninja" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=ON \
    -DLLVM_TARGETS_TO_BUILD=X86 \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DLLVM_ENABLE_ASSERTIONS=ON \
    -DLLVM_ENABLE_SPHINX=ON \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_C_COMPILER=clang \
    -DLLVM_USE_LINKER=lld \
    -fuse-ld=lld \
    ../llvm

If you want to build the analyzer and nothing else, compile the target ``clang``. For development purposes, compile ``check-clang-analysis``.

Invocation
**********

Currently, the only Static Analyzer related command line option for the driver (without the use of ``-cc1`` or ``-Xclang``) is ``--analyze``. This will run the analyzer on the supplied files with a default configuration.

If you'd like to configure the analyzer, you can view the options that belong to clang's frontend via ``clang -cc1 --help | grep analyze``. The minimum you'll need for running the analyzer with `-cc1`:

.. code-block:: bash
  clang -cc1 -analyze -analyzer-checker=core filename.c

Although we don't support running the analyzer without enabling the entire core package, it is possible, but might lead to crashes and incorrect reports.

Initializing the analyzer
*************************

The following section is always subject to change!

First, ``ParseAnalyzerArgs`` in ``(clang repository)/lib/Frontend/CompilerInvocation.cpp`` parses every analyzer related command line configurations, and validates them, with the exception of checker options.

Later, in ``(clang repository)/lib/FrontendTool/ExecuteCompilerInvocation.cpp``, ``AnalysisAction`` is created, which creates an ``AnalysisConsumer``. It's constructor will inspect ``AnalyzerOptions`` and set up all initialization functions according to it. These functions will be called in ``AnalysisConsumer::Initialize``, which will create all the necessary classes needed for the actual analysis. The most important among these is ``CheckerManager`` and ``AnalysisManager``.

``CheckerManager`` owns every checker object, and it's interface allows ``AnalysisManager`` to run specific checkers on specific events. The most important part of it's initialization is loading, or in other terms, registering checkers into it.

Checker registration is handled mostly by the ``CheckerRegistry`` class, which is constructed specifically for ``CheckerManager``'s initialization, and is destructed right after it. After that, ``AnalyzerOptions`` is also regarded as fully initialized, as ``CheckerRegistry`` also validates all checker options.

The actual analysis begins after ``AnalysisConsumer::Initialize()`` is executed.

Terminology
------------

Common file names
^^^^^^^^^^^^^^^^^^

The short file names (as of writing this document) will refer to the following files:

.. _Checkers.td:

* ``Checkers.td``: ``(clang repository)/include/clang/StaticAnalyzer/Checkers/Checkers.td``

.. _Checkerbase.td:

* ``Checkerbase.td``: ``(clang repository)/include/clang/StaticAnalyzer/Checkers/CheckerBase.td``

.. _Checkers.inc:

* ``Checkers.inc``: ``(build directory)/tools/clang/include/clang/StaticAnalyzer/Checkers/Checkers.inc``
.. _ClangSACheckersEmitter.cpp
* ``ClangSACheckersEmitter.cpp`` : ``(clang repository)/utils/TableGen/ClangSACheckersEmitter.cpp``

"Registering a checker"
^^^^^^^^^^^^^^^^^^^^^^^

The term "registering" will be used quite a bit in this document, so it's important to note that what we actually mean under it. Unfortunately, in the code, "registering a checker" can misleadingly mean a couple different things, like

* When ``CheckerManager::registerChecker`` is called, which is what we will refer to, when saying "registering a checker",
* When you add a new entry to Checkers.td_, we will call this "making an entry for a builtin checker",
* When ``CheckerRegistry::addChecker`` is called, we will call this "adding a checker".

"Builtin" and "plugin" checkers
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

We call a checker "builtin", if it has an entry in Checkers.td_. A checker is a "plugin checker", if it was loaded from a plugin runtime. 

Creating a new builtin checker is an easy process, as the code required for adding a checker, ensuring that it's dependencies are registered beforehand, and few other things are generated from the TableGen file according to the entry that was made for it.

The analyzer also supports loading plugins runtime, but that does come at the cost of having to do these things manually.

There is a third category of checkers in this regard, that do not have an entry in the TableGen file, but neither is a plugin checker, for example in ``(clang repository)/unittests/StaticAnalyzer/RegisterCustomCheckersTest.cpp``. These go through the same process are builtin checkers, but without the code being generated for them.

The use of TableGen for builtin checkers
----------------------------------------

During the compilation of the analyzer, Checkers.td_ will be processed by TableGen, which will generate the Checkers.inc_ file according to how the generation was specified in ``(clang repository)/utils/TableGen/ClangSACheckersEmitter.cpp``. CheckerBase.td_ (basically the header file of Checkers.td_) defines the actual structure of a checker entry.

The package system
^^^^^^^^^^^^^^^^^^

Packages are used to bundle checkers into logical categories. Every checker is a part of a package, and any package can be a subpackage of another. If checker ``X`` is within the package ``Y``, its *full name* is ``Y.X``, and it's *name* is ``X``.

Just like a checker, *builtin plugins* can be registered in Checkers.td_, and can be enabled (making every checker inside the package, and every subpackage enabled), or disabled, but only if the package is non-empty. For example, the following entries define the "core" package, and a subpackage of it, the "core.builtin" package.

.. code-block:: c++

  def Core : Package<"core">;
  def CoreBuiltin : Package<"builtin">, ParentPackage<Core>;

We'll define checkers inside packages:

.. code-block:: c++

  let ParentPackage = CoreBuiltin in {
  
  // List of checker entries for the "core.builtin" package...
  
  } // end "core.builtin"

Checker entries
^^^^^^^^^^^^^^^

.. code-block:: c++

  def ClassName : Checker<"CheckerName">,
    HelpText<"Description">,
    Dependencies<[AnotherClassName]>,
    Documentation<DocumentationStateSpecifier>;

An entry will have

* *Class name*, that will be used for function name generation,
* *Checker name*, that specifies the name of the checker, which will be used to generate the checker's full name,
* *Description*, which will be displayed for ``-analyzer-checker-help``,
* (optional) *Dependencies*, which specifies that what other checkers need to be registered before the current one,
* *Documentation state specifier*, which specifies whether the checker has documentation, and is needed for certain output types.

Runtime: from invoking the analyzer to the beginning of the analysis
--------------------------------------------------------------------

The CheckerRegistry class
^^^^^^^^^^^^^^^^^^^^^^^^^

This class is responsible for parsing the generated ``Checkers.inc`` file and registering the checkers into the ``CheckerManager`` class accordingly. This is done by creating a ``CheckerRegistry::CheckerInfo`` object for each entry.
