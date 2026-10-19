# Arsenal program info

Branding and platform metadata consumed by the build:

- Application name, icons, and the macOS asset catalog (`Arsenal.icon/`)
- Desktop entry, AppStream metainfo, and MIME type (`org.arsenallauncher.Arsenal.*`)
- Windows resources (`arsenal.rc.in`, `arsenal.manifest.in`, `win_install.nsi.in`)
- Man page source (`arsenal.6.scd.in`), built when `scdoc` is available

`CMakeLists.txt` in this directory sets the Arsenal app id (`org.arsenallauncher.Arsenal`), display name, and git URL. Prism-named files are still present for upstream history; the branding target uses the Arsenal files.
