#
# project: battle
#
cmake_minimum_required(VERSION 2.8)

# include the fips main cmake file
get_filename_component(FIPS_ROOT_DIR "../fips" ABSOLUTE)
include("${FIPS_ROOT_DIR}/cmake/fips.cmake")

fips_setup()
fips_project(battle)
fips_add_subdirectory(code)

# manually defined imports, we don't need all of Oryol
fips_ide_group("Oryol")
fips_import_oryol_Core()
fips_import_oryol_Gfx()
fips_import_oryol_Assets()
fips_import_oryol_Resource()
fips_import_oryol_Input()
fips_import_oryol_IO()
fips_import_oryol_HttpFS()
fips_import_oryol_LocalFS()
fips_ide_group("Libs")
fips_import_fips_glfw_glfw3()
fips_ide_group("")

fips_finish()
