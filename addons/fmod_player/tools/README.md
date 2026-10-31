# FMOD localization tools

Open **Project > Tools > FMOD Translations**, or run `generate_property_translations.tscn` directly.

| Catalog | Domain | Source |
| --- | --- | --- |
| Properties | `godot.properties` | `translations/fmod_property_translations.csv` |
| Editor Interface | `godot.editor` | `translations/fmod_editor_translations.csv` |
| API Documentation | `godot.documentation` | `translations/documentation/*.po` |

Select a language and edit entries in the detail panel. Search and the untranslated/review filter narrow the list. Save validates the data and refreshes the shared loader. Empty translations stay in the CSV and use the normal fallback. Multiline CSV cells use CSV quoting; backslashes remain literal.

PO editing and source extraction require Python and Babel. From the source repository, install `tools/requirements-docs.txt`, then set the Python executable and source repository path in the panel. These settings are saved per project. The addon itself loads translations through Godot without Python. API source keys are read-only; the Fuzzy checkbox marks translations that need review.

Property extraction reads ClassDB. UI extraction scans marked C++ and GDScript text and scene UI text in the source repository. API extraction updates the PO from XML using the existing documentation tools. Review extracted entries before saving. Keep all four UI languages and format placeholders complete.

Edit the panel layout and signal connections in `generate_property_translations.tscn`. Its script handles catalog data and interaction through named scene nodes.

The legacy `generate_property_translations.gd --csv <path>` command validates a CSV. Translation data GDScript is no longer generated.
