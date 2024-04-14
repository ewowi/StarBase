Import('env')
import os
import shutil
import gzip

print("")
print("***************************************************************************************************************************")
print("********** STARMOD IS LICENSED UNDER GPL-V3. BY INSTALLING STARMOD YOU IMPLICITLY ACCEPT THE TERMS AND CONDITIONS  ********")
print("***************************************************************************************************************************")
print("")

OUTPUT_DIR = os.path.expanduser("~")+"/Downloads/"

def _get_cpp_define_value(env, define):
    define_list = [item[-1] for item in env["CPPDEFINES"] if item[0] == define]

    if define_list:
        return define_list[0]

    return None

def bin_rename_copy(source, target, env):
    app = _get_cpp_define_value(env, "APP")
    version = _get_cpp_define_value(env, "VERSION")
    pioenv = env["PIOENV"]

    # create string with location and file names based on pioenv
    map_file = "{}{}_{}_{}.map".format(OUTPUT_DIR, app, version, pioenv)
    bin_file = "{}{}_{}_{}.bin".format(OUTPUT_DIR, app, version, pioenv)
    gzip_file = "{}{}_{}_{}.bin.gz".format(OUTPUT_DIR, app, version, pioenv)

    # check if new target files exist and remove if necessary
    for f in [map_file, bin_file]:
        if os.path.isfile(f):
            os.remove(f)

    # copy firmware.bin to bin_file
    shutil.copy(str(target[0]), bin_file)
    print("  created " + bin_file)

    # copy firmware.map to map_file
    if os.path.isfile("firmware.map"):
        shutil.move("firmware.map", map_file)

    # do not create zip version (yet)

    # # check if new target files exist and remove if necessary
    # if os.path.isfile(gzip_file): os.remove(gzip_file)

    # # write gzip firmware file
    # with open(bin_file,"rb") as fp:
    #     with gzip.open(gzip_file, "wb", compresslevel = 9) as f:
    #         shutil.copyfileobj(fp, f)
    #         print("  created " + gzip_file)

env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", [bin_rename_copy])
