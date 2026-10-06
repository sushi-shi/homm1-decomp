# The `heroes` and `heroes-editor` launchers (nix/game.nix). Embedded by
# writeShellApplication, which sets program (the native executable), title,
# store_data (the game data laid out in the store at install time, or empty),
# importer (nix/game-data.py) and state_name (the per-user folder's name).
#
# The per-user folder ($XDG_DATA_HOME/$state_name):
#   game/   the folder the program runs on. ANIM, SOUND and HELP link to the
#           read-only data; DATA's resource archive is a link too. The files
#           the programs write (saved games in GAMES, maps in MAPS, high scores
#           and the network save in DATA) are copied from the data once and
#           are never overwritten or brought back after being deleted.
#   data/   the game data imported from $HOMM1_GAME on first run, when the
#           package was installed without it.
# Settings and the converted help stay in $XDG_CONFIG_HOME/homm1.

for argument in "$@"; do
  case "$argument" in
    --help|-h)
      printf '%s\n' \
        "$title. Game data: ${store_data:-HOMM1_GAME=PATH on first run (the Buka 2003 CD image, CD or installed game folder, or a .zip/.7z/.rar)}" \
        "Writable files (saves, maps, high scores) live in \$XDG_DATA_HOME/$state_name/game; settings in \$XDG_CONFIG_HOME/homm1." \
        "--data DIR or HOMM1_DATA runs on DIR as it is. The program's own options:"
      exec "$program" --help
      ;;
    --data)
      exec "$program" "$@"
      ;;
  esac
done
if [[ -n "${HOMM1_DATA:-}" ]]; then
  exec "$program" "$@"
fi

case "${XDG_DATA_HOME:-}" in
  /*) state="$XDG_DATA_HOME/$state_name" ;;
  *) state="${HOME:?HOME or an absolute XDG_DATA_HOME is required}/.local/share/$state_name" ;;
esac
mkdir -p -- "$state"
exec {lock}>"$state/.lock"
flock -x "$lock"

if [[ -n "$store_data" ]]; then
  layer="$store_data"
else
  layer="$state/data"
  source_file="$layer/.source"
  if [[ ! -f "$source_file" || ( -n "${HOMM1_GAME:-}" && "$(cat -- "$source_file")" != "$HOMM1_GAME" ) ]]; then
    if [[ -z "${HOMM1_GAME:-}" ]]; then
      printf '%s\n' "First run: set HOMM1_GAME to your Buka 2003 game (its CD image, the CD, an installed game folder, or a .zip/.7z/.rar of one), e.g." \
        "  HOMM1_GAME=~/Games/heroes.iso $(basename -- "$0")" \
        "Later runs use the copy imported into $layer." >&2
      exit 1
    fi
    printf 'Importing the game from %s\n' "$HOMM1_GAME" >&2
    rm -rf -- "$state/data.partial"
    if ! "$importer" --game "$HOMM1_GAME" --out "$state/data.partial" --work "$state"; then
      rm -rf -- "$state/data.partial"
      exit 1
    fi
    printf '%s' "$HOMM1_GAME" > "$state/data.partial/.source"
    find "$state/data.partial" -type f -exec chmod a-w {} +
    rm -rf -- "$layer"
    mv -T -- "$state/data.partial" "$layer"
  fi
fi

# A link into the data, replacing an older link (a previous store path) but
# never a file or folder of the player's.
link() {
  if [[ -L "$2" || ! -e "$2" ]]; then
    ln -sfn -- "$1" "$2"
  fi
}

game="$state/game"
seeded="$game/.seeded"
mkdir -p -- "$game"
touch -- "$seeded"
for directory in "$layer/game"/*/; do
  name=$(basename -- "$directory")
  case "$name" in
    ANIM|SOUND|HELP)
      link "$layer/game/$name" "$game/$name"
      continue
      ;;
  esac
  mkdir -p -- "$game/$name"
  for file in "$layer/game/$name"/*; do
    base=$(basename -- "$file")
    target="$game/$name/$base"
    if [[ "$name/${base,,}" == DATA/heroes.agg ]]; then
      link "$file" "$target"
    elif ! grep -qxF -- "$name/$base" "$seeded"; then
      if [[ ! -e "$target" && ! -L "$target" ]]; then
        cp -- "$file" "$target"
        chmod u+w -- "$target"
      fi
      printf '%s\n' "$name/$base" >> "$seeded"
    fi
  done
done

if [[ -z "${HOMM1_CD:-}" && -d "$layer/cd/Tracks" ]]; then
  export HOMM1_CD="$layer/cd"
fi
flock -u "$lock"
exec {lock}>&-
exec "$program" --data "$game" "$@"
