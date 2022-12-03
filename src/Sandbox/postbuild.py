import glob
import os
import shutil
from pathlib import Path

# extenions example '*.bmp'
# copies files based on extension and recreates folder structure inside destination
def copy(src, dest, ext):
    for file_path in glob.glob(os.path.join(src, '**', ext), recursive=True):
        fdpath = dest + file_path[len(src):]
        if Path(Path(fdpath).parent).exists() == False:
            os.makedirs(Path(fdpath).parent)
        x = Path(fdpath)
        new_path = os.path.join(x.parent, os.path.basename(file_path))
        print("{0} --> {1}".format(file_path, new_path))
        shutil.copy(file_path, new_path)

print('Copying library files')
copy("../../bin/CompGFX/Debug-x86_64/", "../../bin/Sandbox/Debug-x86_64/", "*.*")
copy("../../bin/CompGFX/Release-x86_64/", "../../bin/Sandbox/Release-x86_64/", "*.*")