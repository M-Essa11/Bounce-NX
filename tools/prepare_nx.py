"""Copy the game's assets into the embedded NX RomFS. Standard library only."""
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parents[1]
ASSET_DIRS = ("icons", "levels", "sounds", "lang", "fonts")

def prepare():
    for directory in ASSET_DIRS:
        shutil.copytree(ROOT / directory, ROOT / "romfs-nx" / directory, dirs_exist_ok=True)

if __name__ == "__main__":
    prepare()
