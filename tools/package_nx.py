"""Verify embedded assets and create the microSD-ready test ZIP."""
from pathlib import Path
import hashlib
import json
import struct
import zipfile
from prepare_nx import ROOT, ASSET_DIRS

def sha(data):
    return hashlib.sha256(data).hexdigest()

def validate():
    nro = (ROOT / "bounce-nx.nro").read_bytes()
    assert nro[0x10:0x14] == b"NRO0", "Invalid NRO header"
    image_size = struct.unpack_from("<I", nro, 0x18)[0]
    assert nro[image_size:image_size + 4] == b"ASET", "Missing NRO assets"
    icon_off, icon_size, nacp_off, nacp_size, romfs_off, romfs_size = struct.unpack_from(
        "<6Q", nro, image_size + 8)
    assert image_size + romfs_off + romfs_size <= len(nro)
    assert nro[image_size + icon_off:image_size + icon_off + 2] == b"\xff\xd8"
    nacp = nro[image_size + nacp_off:image_size + nacp_off + nacp_size]
    assert nacp[:0x200].split(b"\0", 1)[0] == b"Bounce NX"
    assert nacp[0x200:0x300].split(b"\0", 1)[0] == b"M-Essa11"
    fs = nro[image_size + romfs_off:image_size + romfs_off + romfs_size]
    header = struct.unpack_from("<10Q", fs)
    assert header[0] == 0x50
    dir_table, file_table, file_data = header[3], header[7], header[9]
    embedded = {}
    def visit(offset, prefix):
        parent, sibling, child_dir, child_file, hnext, name_len = struct.unpack_from(
            "<6I", fs, dir_table + offset)
        name = fs[dir_table + offset + 24:dir_table + offset + 24 + name_len].decode()
        path = f"{prefix}/{name}".strip("/")
        while child_file != 0xFFFFFFFF:
            entry = file_table + child_file
            p, child_file, data_off, size, hn, length = struct.unpack_from("<IIQQII", fs, entry)
            filename = fs[entry + 32:entry + 32 + length].decode()
            content = fs[file_data + data_off:file_data + data_off + size]
            assert len(content) == size
            embedded[f"{path}/{filename}".strip("/")] = content
        while child_dir != 0xFFFFFFFF:
            visit(child_dir, path)
            child_dir = struct.unpack_from("<I", fs, dir_table + child_dir + 4)[0]
    visit(0, "")
    expected = {
        f.relative_to(ROOT).as_posix(): f.read_bytes()
        for directory in ASSET_DIRS for f in (ROOT / directory).rglob("*") if f.is_file()
    }
    assert embedded.keys() == expected.keys(), "Embedded file list differs from source assets"
    for name, content in expected.items():
        assert embedded[name] == content, f"Asset mismatch: {name}"
    assert sum(name.startswith("levels/J2MElvl.") for name in embedded) == 11
    return nro, {
        "nro_sha256": sha(nro), "nro_bytes": len(nro),
        "embedded_files_verified": len(embedded), "levels_verified": 11,
        "nacp_title": "Bounce NX", "hardware_tested": False,
        "source_base": "M-Essa11/Bounce-Vita with Super-Matter's physics PR #1 (13e5705)",
        "asset_sha256": {name: sha(data) for name, data in sorted(embedded.items())},
    }

if __name__ == "__main__":
    nro, report = validate()
    dist = ROOT / "dist"
    dist.mkdir(exist_ok=True)
    package = dist / "Bounce-NX-v0.1.2-test.zip"
    report_text = json.dumps(report, indent=2) + "\n"
    (dist / "validation.json").write_text(report_text)
    with zipfile.ZipFile(package, "w", zipfile.ZIP_DEFLATED) as archive:
        archive.writestr("switch/bounce-nx/bounce-nx.nro", nro)
        archive.write(ROOT / "README.md", "INSTALL.md")
        archive.write(ROOT / "LICENSE", "LICENSE")
        archive.write(ROOT / "NOTICE.md", "NOTICE.md")
        archive.writestr("validation.json", report_text)
    with zipfile.ZipFile(package) as archive:
        assert archive.testzip() is None
        assert archive.read("switch/bounce-nx/bounce-nx.nro") == nro
    print(json.dumps({k: v for k, v in report.items() if k != "asset_sha256"}, indent=2))
    print(package)
