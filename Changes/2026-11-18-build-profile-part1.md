- Moved the toolset (`v145`), the Windows SDK (`10.0.26100.0`), the character set, the debug-library choice, the
  runtime library and the optimization level from `RedTilde/RedTilde.vcxproj` into `Directory.Build.props`, so the
  project file holds only its own facts (`RedHome` convergence CV-6-04 part 1, register C-11).
  `Build/NativeBuildPolicy.targets` now has the text of the DxUi commit that `Dependencies/DxUi.lock.json` pins
  (`21fae0dc`), the copy that compiles the DxUi archive (C-02); it adds one include folder for DxUi's own projects
  only. The DxUi build identity hashes this file, so the next build rebuilds the DxUi archive once.
  No compiler or linker input of RedTilde changes: an MSBuild evaluation of the project in the six profiles before
  and after shows the same properties read by the build targets and the same metadata on every compile, link,
  resource, manifest and build-event item.
