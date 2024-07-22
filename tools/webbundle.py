Import("env")
import shutil

cmd_ex = shutil.which("node")
# Check if Node.js is not installed
if cmd_ex is None:
    print('\x1b[0;31;43m' + 'Node.js is not installed or missing from PATH changes to html css js will not be processed' + '\x1b[0m')
else:
    # Install the necessary node packages for the pre-build asset bundling script
    print('\x1b[6;33;42m' + 'Install the node packages' + '\x1b[0m')

    # npm ci performs a clean install of all existing dependencies
    env.Execute("npm ci")

    # Call the bundling script
    exitCode = env.Execute("node tools/cdata.js")

    # If it failed, abort the build
    if (exitCode):
      Exit(exitCode)
