# Godot API

The bindings target Godot 4.7.2 stable, single precision. `SConstruct` passes
`godot_4_7_2.json` to godot-cpp through `custom_api_file`.

To refresh the snapshot, export with the matching Godot executable using
`--headless --dump-extension-api`, then run:

```sh
python tools/update_godot_api.py /path/to/extension_api.json api/godot_4_7_2.json
```

The normalizer omits constants already supplied by `<cstdint>` to support the
pinned godot-cpp generator. It preserves engine classes, signatures and hashes.
