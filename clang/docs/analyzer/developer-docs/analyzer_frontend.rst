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
    -DSPHINX_WARNINGS_AS_ERROS=OFF \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_C_COMPILER=clang \
    -DLLVM_USE_LINKER=lld \
    -fuse-ld=lld \
    ../llvm

If you want to build the analyzer and nothing else, compile the target ``clang``. For development purposes, compile ``check-clang-analysis``.

Invocation
**********

Currently, the only Static Analyzer related command line option for the driver (without the use of ``-cc1`` or ``-Xclang``) is ``--analyze``. This will run the analyzer on the supplied files with a default configuration.

If you'd like to configure the analyzer, you can view the options that belong to clang's frontend via ``clang -cc1 --help | grep analyze``. The minimum you'll need for running the analyzer with ``-cc1``:

.. code-block:: bash

  clang -cc1 -analyze -analyzer-checker=core filename.c

Although we don't support running the analyzer without enabling the entire core package, it is possible, but might lead to crashes and incorrect reports.

Initializing the analyzer
*************************

The following section is always subject to change!

First, ``ParseAnalyzerArgs`` in ``(clang repository)/lib/Frontend/CompilerInvocation.cpp`` parses every analyzer related command line arguments, validates them, with the exception of checker options.

Later, in ``(clang repository)/lib/FrontendTool/ExecuteCompilerInvocation.cpp``, ``AnalysisAction`` is created, which creates an ``AnalysisConsumer``. It's constructor will inspect ``AnalyzerOptions`` and set up all initialization functions according to it. These functions will be called in ``AnalysisConsumer::Initialize``, which will create all the necessary classes needed for the actual analysis. The most important among these is ``CheckerManager`` and ``AnalysisManager``.

``CheckerManager`` owns every checker object, and it's interface allows ``AnalysisManager`` to run specific checkers on specific events. The most important part of it's initialization is loading, or in other terms, registering checkers into it.

Checker registration is handled mostly by the ``CheckerRegistry`` class, which is constructed specifically for ``CheckerManager``'s initialization, and is destructed right after it. After that, ``AnalyzerOptions`` is also regarded as fully initialized, as ``CheckerRegistry`` also validates all checker options.

The actual analysis begins after ``AnalysisConsumer::Initialize()`` is executed.

Checkers and checker registration
---------------------------------

This section will detail

* What we actually mean under the term "checker",
* How are they registered (and what registering actually means!),
* How can the user load checkers from plugins,
* How can we establish dependencies in between checkers,
* How can we add checker options.

If you are only developing a single checker, chances are that you won't need to read this entire document. However, if you are a long term developer of maintainer in the Static Analyzer, the more you know the better.

Terminology
^^^^^^^^^^^

As the analyzer matured over the years, specific terms that described one specific function can now mean a variety of different things. For example, in the early 2010s, we used the term "checks" (similarly to clang-tidy) instead of "checkers", and there still are some remnants of this in class/object names and documentation. Among the most commonly misused words is "registration".

This section aims to clarify most of these things. It will talk about things that will only be detailed later on, so feel free to skip some parts if they are unclear just yet.

Common file names
*****************

The short file names (as of writing this document) will refer to the following files:

.. _Checkers.td:

* ``Checkers.td``: ``(clang repository)/include/clang/StaticAnalyzer/Checkers/Checkers.td``

.. _Checkerbase.td:

* ``Checkerbase.td``: ``(clang repository)/include/clang/StaticAnalyzer/Checkers/CheckerBase.td``

.. _Checkers.inc:

* ``Checkers.inc``: ``(build directory)/tools/clang/include/clang/StaticAnalyzer/Checkers/Checkers.inc``

.. _ClangSACheckersEmitter.cpp:

* ``ClangSACheckersEmitter.cpp`` : ``(clang repository)/utils/TableGen/ClangSACheckersEmitter.cpp``

"Registering a checker"
***********************

The term "registering" will be used quite a bit in this document, so it's important to note that what we actually mean under it. Unfortunately, in the code, "registering a checker" can misleadingly mean a couple different things, like

* When ``CheckerManager::registerChecker`` is called, which is what we will refer to, when saying "registering a checker",
* When you add a new entry to Checkers.td_, we will call this "making an entry for a builtin checker",
* When ``CheckerRegistry::addChecker`` is called, we will call this "adding a checker".

Checkers
********

Checkers are basically the bread and butter of the analyzer. When specific events (such as a call to a function) happen, checkers may register to that event by implementing a callback (a method), that will be called.

The parts of a checker
""""""""""""""""""""""

Most checkers have their own file in ``(clang repository)/lib/StaticAnalyzer/Checkers/``, which will contain a *checker class* on the top and a *checker registry function* on the bottom. The latter creates a single instance of the checker class called the *checker object*, which is owned by ``CheckerManager``.

A *package* is not much more than a single string, used for bundling checkers into logical categories. Every checker is a part of a package, and any package can be a *subpackage* of another. If package ``builtin`` is a subpackge of ``core``, it's *full name* will be ``core.builtin``, and it's *name* will be ``builtin``. Similarly if checker ``X`` is within the package ``Y``, its *full name* is ``Y.X``, and it's *name* is ``X``.

Checker dependencies
""""""""""""""""""""

Checkers can depend on one another. If a dependency is disabled, so must be every checker that depends on it.

Should we imagine checker dependencies as a graph, it would be a directed forest, where the nodes are checkers: each directed tree describes a group of checker's dependencies, a node's parent would be it's dependency, and is ensured to be registered before it's children.

Currently, we don't allow directed circles within this graph, but it would certainly be a great addition.

"Builtin" and "plugin" checkers
"""""""""""""""""""""""""""""""

We call a checker *builtin*, if it has an entry in Checkers.td_. A checker is a *plugin checker*, if it was loaded from a plugin runtime. 

There is a third category of checkers in this regard, that do not have an entry in the TableGen file, but neither is a plugin checker, for example in ``(clang repository)/unittests/StaticAnalyzer/RegisterCustomCheckersTest.cpp``. These go through the same process are builtin checkers, but without the code being generated for them.

Similarly, *builtin packages* have an entry in Checkers.td_, and *plugin packages* are loaded from a plugin runtime.

Subcheckers
"""""""""""

As stated earlier, *most* checkers have a single checker object, but not all. *Subcehckers* do not have one on their own, as they are most commonly built in another checker that does. For example, many checkers are implemented by having a checker object which models something (like dynamic memory allocation), and enabling certain subcheckers of it will make the modeling part emit certain reports (like emitting a report for double delete errors). Practically, subcheckers most of the time can be regarded as checker options to the *main checker*.

Command line options
""""""""""""""""""""

Both checkers and packages can possess *options*. Each package option transitively belongs to all of its subpackages and checkers. These of these options must be preceded by ``-analyzer-config`` and must have the following format:

.. code-block:: bash

  -analyzer-config CheckerOrPackageFullName:OptionName=Value

Should the user supply the same option multiple times (with possibly different values), only the last one will be regarded. If compatibility mode (which is implicitly enabled in driver mode) is disabled, these options will be verified, and additional verifications can be added to the checker's registry function.

Checker registration
^^^^^^^^^^^^^^^^^^^^

The checker registration, or initialization process begins when the ``CheckerRegistry`` object is created. It will store a ``CheckerRegisty::CheckerInfo`` object for each checker containing their full name, a pointer to their checker registry function, and some other things that we will detail later. It'll parse the user's input about which checker should be enabled, resolves dependencies, validates checker options, and eventually calls the checker registry functions by supplying each with a ``CheckerManager`` object. By the time the ``CheckerRegistry`` object is destructed, all necessary checker objects have been created and initialized.

In the following subsections, we'll investigate how one can use ``CheckerRegistry``'s interface to add new checkers and packages to it. For builtin checkers, there's no need to interact with ``CheckerRegistry`` at all.

Registering a package
*********************

A new package can be added via ``CheckerRegistry::addPackage()``, which expect a package full name.

A new package option can be added via ``CheckerRegistry::addPackageOption``, which expects the package's full name, the option's name, the default value of it, a human-readable description and the option's type. You can add several package options to a single package by supplying the same package full name when calling ``addPackageOption`` again.

Registering a checker
*********************

A new checker can be added via the ``CheckerRegisty::addChecker`` template method, which expects a full checker name, a human-readable description, a pointer to the checker registry function, a pointer to the checker's ``shouldRegister`` function, a (preferably existing) link to the checker's documentation page as regular parameters and the checker class as a template parameter.

A new checker option can be added via ``CheckerRegistry::addCheckerOption``, which expects the checker's full name, the option's name, the default value of it, a human-readable description and the option's type. You can add several checker options to a single checker by supplying the same checker full name when calling ``addCheckerOption`` again.

One can establish dependencies in between checkers by calling ``CheckerRegistry::addDependency``, which expects in order the dependendt checker's full name, and the dependency-checker's full name.

Generating code for builtin checkers
************************************

Creating a new builtin checker is an easy process, as the code required for adding a checker, ensuring that it's dependencies are registered beforehand, and few other things are generated from TableGen files according to the entry that was made for it. Usually, adding 5-10 lines to Checkers.td_ is all you need to do.

During the compilation of the analyzer, Checkers.td_ will be processed by TableGen, which will generate the Checkers.inc_ file according to how the generation was specified in ``(clang repository)/utils/TableGen/ClangSACheckersEmitter.cpp``. CheckerBase.td_ (basically the header file of Checkers.td_) defines the actual structure of a checker entry.

Creating a basic entry for a builtin package
""""""""""""""""""""""""""""""""""""""""""""

* *Name*,
* (optional) *Parent package*, which expects a package as an argument. This is how one can express that this entry is a subpacke, and is used for generating the plugin's full name,
* (optional) *Package options* (detailed in a later section).


.. code-block:: c++

  def PackageClassName : Package<"PackageName">;

With all optional fields:

.. code-block:: c++

  def AnotherPackage : Package<"AnotherPackage">,
    ParentPackage<PackageClassName>,
    PackageOptions<[
      CmdLineOption<CommandLineOptionType,
                    "OptionName",
                    "OptionDescription",
                    "DefaultValue">,
      CmdLineOption<CommandLineOptionType2,
                    "OptionName2",
                    "OptionDescription2",
                    "DefaultValue2">,
    ]>;

We'll define checkers inside packages:

.. code-block:: c++

  let ParentPackage = AnotherPackage in {
  
  // List of checker entries for the "core.builtin" package...
  
  } // end "core.builtin"

Creating a basic entry for a builtin checker
""""""""""""""""""""""""""""""""""""""""""""

* *Parent package*, which specified that which package dies this checker belong to. This is assigned implicitly according to which ``let ParentPackage = ??? in { /* checker entry */ }`` block was the checker defined in.
* *Class name*, that will be used for function name generation,
* *Checker name*, that specifies the name of the checker, which will be used to generate the checker's full name,
* *Description*, which will be displayed for ``-analyzer-checker-help``,
* (optional) *Dependencies*, which specifies that what other checkers need to be registered before the current one,
* (optional) Checker options (detailed in a later section).
* *Documentation state specifier*, which specifies whether the checker has documentation, and is needed for certain output types (detailed in a later section).

.. code-block:: c++

  def ClassName : Checker<"CheckerName">,
    HelpText<"Description">,
    Documentation<DocumentationStateSpecifier>;

With all optional fields:

.. code-block:: c++

  def ClassName : Checker<"CheckerName">,
    HelpText<"Description">,
    Dependencies<[AnotherClassName, YetAnotherClassName]>,
    CheckerOptions<[
      CmdLineOption<CommandLineOptionType,
                    "OptionName",
                    "OptionDescription",
                    "DefaultValue">,
      CmdLineOption<CommandLineOptionType2,
                    "OptionName2",
                    "OptionDescription2",
                    "DefaultValue2">,
    ]>,
    Documentation<DocumentationStateSpecifier>;

Creating and loading checker plugins
************************************

Should you choose not to add a checker to the official Clang repository (possibly due to security of confidentiality reasons), you can still create checkers that you can load runtime. These checkers can access the same functionality as regular builtin checkers.

Creating a checker plugin
"""""""""""""""""""""""""

*Checker plugins* can be compiled on their own, but can only be used with a specific clang version. At the very least, it is a dynamic library that exports ``clang_analyzerAPIVersionString``. This should be defined as follows:

.. code-block:: c++

  extern "C"
  const char clang_analyzerAPIVersionString[] =
      CLANG_ANALYZER_API_VERSION_STRING;

This is used to check whether the current version of the analyzer compatible with the plugin. Attempting to load plugins with incompatible version strings, or without a version string at all, will result in warnings and the plugins not being loaded.

To add a custom checker to the analyzer, the plugin must also define the function ``clang_registerCheckers``.

.. code-block:: c++

   extern "C"
   void clang_registerCheckers (CheckerRegistry &registry) {
     registry.addChecker<MainCallChecker>(
         "example.MainCallChecker", "Disallows calls to functions called main");

     // Register more checkers, plugins, checker dependencies, options...
   }

The first method argument is the full name of the checker, including its enclosing package. The second method argument is a short human-readable description of the checker.

The ``clang_registerCheckers`` function may add any number of checkers to the registry. If any checkers require additional initialization, use the three-argument form of CheckerRegistry::addChecker.

To load a checker plugin, specify the full path to the dynamic library as the argument to the ``-load`` option to the frontend.

.. code-block:: bash

  clang -cc1 -load </path/to/plugin.dylib> -analyze
    -analyzer-checker=<example.MainCallChecker>

  clang -Xclang -load -Xclang </path/to/plugin.dylib> --analyze
    -Xclang -analyzer-checker=example.MainCallChecker

You can then enable your custom checker using the ``-analyzer-checker``:
For a complete working example, see examples/analyzer-plugin.

Establishing dependencies in between checkers
*********************************************

If a checker depends on another (for example, DynamicMemoryModeling depends on CStringModeling to model calls to ``strcmp``), these dependencies can be declared in Checkers.td_,
