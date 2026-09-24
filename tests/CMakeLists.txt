project(tests)

ecm_add_test(FileSystem_test.cpp LINK_LIBRARIES Launcher_logic Qt${QT_VERSION_MAJOR}::Test
    TEST_NAME FileSystem)

ecm_add_test(GZip_test.cpp LINK_LIBRARIES Launcher_logic Qt${QT_VERSION_MAJOR}::Test
    TEST_NAME GZip)

ecm_add_test(GradleSpecifier_test.cpp LINK_LIBRARIES Launcher_logic Qt${QT_VERSION_MAJOR}::Test
    TEST_NAME GradleSpecifier)

ecm_add_test(MojangVersionFormat_test.cpp LINK_LIBRARIES Launcher_logic Qt${QT_VERSION_MAJOR}::Test
    TEST_NAME MojangVersionFormat)

ecm_add_test(Library_test.cpp LINK_LIBRARIES Launcher_logic Qt${QT_VERSION_MAJOR}::Test
    TEST_NAME Library)

ecm_add_test(ResourceFolderModel_test.cpp LINK_LIBRARIES Launcher_logic Qt${QT_VERSION_MAJOR}::Test
    TEST_NAME ResourceFolderModel)

ecm_add_test(ResourcePackParse_test.cpp LINK_LIBRARIES Launcher_logic Qt${QT_VERSION_MAJOR}::Test
    TEST_NAME ResourcePackParse)

ecm_add_test(TexturePackParse_test.cpp LINK_LIBRARIES Launcher_logic Qt${QT_VERSION_MAJOR}::Test
    TEST_NAME TexturePackParse)

ecm_add_test(DataPackParse_test.cpp LINK_LIBRARIES Launcher_logic Qt${QT_VERSION_MAJOR}::Test
    TEST_NAME DataPackParse)

ecm_add_test(ShaderPackParse_test.cpp LINK_LIBRARIES Launcher_logic Qt${QT_VERSION_MAJOR}::Test
    TEST_NAME ShaderPackParse)

ecm_add_test(WorldSaveParse_test.cpp LINK_LIBRARIES Launcher_logic Qt${QT_VERSION_MAJOR}::Test
    TEST_NAME WorldSaveParse)

ecm_add_test(ParseUtils_test.cpp LINK_LIBRARIES Launcher_logic Qt${QT_VERSION_MAJOR}::Test
    TEST_NAME ParseUtils)

ecm_add_test(Task_test.cpp LINK_LIBRARIES Launcher_logic Qt${QT_VERSION_MAJOR}::Test
    TEST_NAME Task)

ecm_add_test(INIFile_test.cpp LINK_LIBRARIES Launcher_logic Qt${QT_VERSION_MAJOR}::Test
    TEST_NAME INIFile)

ecm_add_test(JavaVersion_test.cpp LINK_LIBRARIES Launcher_logic Qt${QT_VERSION_MAJOR}::Test
    TEST_NAME JavaVersion)

ecm_add_test(Packwiz_test.cpp LINK_LIBRARIES Launcher_logic Qt${QT_VERSION_MAJOR}::Test
    TEST_NAME Packwiz)

ecm_add_test(Index_test.cpp LINK_LIBRARIES Launcher_logic Qt${QT_VERSION_MAJOR}::Test
    TEST_NAME Index)

ecm_add_test(Version_test.cpp LINK_LIBRARIES Launcher_logic Qt${QT_VERSION_MAJOR}::Test
    TEST_NAME Version)

ecm_add_test(MetaComponentParse_test.cpp LINK_LIBRARIES Launcher_logic Qt${QT_VERSION_MAJOR}::Test
    TEST_NAME MetaComponentParse)

ecm_add_test(XmlLogs_test.cpp LINK_LIBRARIES Launcher_logic Qt${QT_VERSION_MAJOR}::Test
    TEST_NAME XmlLogs)

ecm_add_test(Commandline_test.cpp LINK_LIBRARIES Launcher_logic Qt${QT_VERSION_MAJOR}::Test
    TEST_NAME Commandline)

ecm_add_test(AuthParsers_test.cpp LINK_LIBRARIES Launcher_logic Qt${QT_VERSION_MAJOR}::Test
    TEST_NAME AuthParsers)

ecm_add_test(RuntimeContext_test.cpp LINK_LIBRARIES Launcher_logic Qt${QT_VERSION_MAJOR}::Test
    TEST_NAME RuntimeContext)

ecm_add_test(InstanceCopyPrefs_test.cpp LINK_LIBRARIES Launcher_logic Qt${QT_VERSION_MAJOR}::Test
    TEST_NAME InstanceCopyPrefs)

ecm_add_test(NetUtils_test.cpp LINK_LIBRARIES Launcher_logic Qt${QT_VERSION_MAJOR}::Test
    TEST_NAME NetUtils)

ecm_add_test(MessageLevelLine_test.cpp LINK_LIBRARIES Launcher_logic Qt${QT_VERSION_MAJOR}::Test
    TEST_NAME MessageLevelLine)

ecm_add_test(AuthSession_test.cpp LINK_LIBRARIES Launcher_logic Qt${QT_VERSION_MAJOR}::Test
    TEST_NAME AuthSession)
