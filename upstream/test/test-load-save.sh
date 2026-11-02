set -ex

bygfoot_bin=$1
bygfoot_bindir=`dirname $bygfoot_bin`

function diff_saves {
    save0=$1
    save1=$2
    test -z "$(sqldiff $save0 $save1 | sed -e '/^CREATE INDEX/d' -e '/^DROP INDEX/d')"
}


json_file=`mktemp`
save_dir=`mktemp -d`

cat <<EOF > $json_file
{ 'commands' : [
  { 'add_user' : { 'username' : 'test', 'country' : 'faroe_islands', 'team' : 'V. Gotu' }},
  { 'start_bygfoot' : {}},
  { 'simulate_games' : {'weeks' : 5}},
  { 'save_bygfoot' : {'filename' : '$save_dir/save0.sqlite'}},
  { 'load_bygfoot' : {'filename' : '$save_dir/save0.sqlite'}},
  { 'save_bygfoot' : {'filename' : '$save_dir/save1.sqlite'}},
  { 'simulate_games' : {'years' : 1}},
  { 'save_bygfoot' : {'filename' : '$save_dir/save2.sqlite'}},
  { 'load_bygfoot' : {'filename' : '$save_dir/save2.sqlite'}},
  { 'save_bygfoot' : {'filename' : '$save_dir/save3.sqlite'}},
]}
EOF

echo $save_dir
tmphome=`mktemp -d`
pushd $bygfoot_bindir
HOME=$tmphome ./bygfoot --random-seed=1 --json=$json_file

# Verify that the save file are identical.
diff_saves $save_dir/save0.sqlite $save_dir/save1.sqlite
diff_saves $save_dir/save2.sqlite $save_dir/save3.sqlite

# Test that we can load the saves on start up.
cat << EOF > $json_file
{ 'commands' : [
  {'load_bygfoot' : {'filename' : '$save_dir/save0.sqlite'}},
  {'load_bygfoot' : {'filename' : '$save_dir/save2.sqlite'}}
]}
EOF

HOME=$tmphome ./bygfoot --random-seed=1 --json=$json_file

# Test that the save files are correct after exactly one season.  The
# game is in a unique state after the beginning of a season so we need
# to make sure we can load and save this.
cat << EOF > $json_file
{ 'commands' : [
  { 'add_user' : { 'username' : 'test', 'country' : 'faroe_islands', 'team' : 'V. Gotu' }},
  { 'start_bygfoot' : {}},
  { 'simulate_games' : {'years' : 1}},
  { 'save_bygfoot' : {'filename' : '$save_dir/one-season.sqlite'}},
  { 'dump_bygfoot' : {}}
]}
EOF

HOME=$tmphome ./bygfoot --random-seed=1 --json=$json_file > save.oneseason.log

cat << EOF > $json_file
{ 'commands' : [
  { 'load_bygfoot' : {'filename' : '$save_dir/one-season.sqlite'}},
  { 'dump_bygfoot' : {}},
  { 'simulate_games' : {'years' : 1}}
]}
EOF

HOME=$tmphome ./bygfoot --random-seed=1 --json=$json_file > load.oneseason.log

if ! diff -q save.oneseason.log load.oneseason.log; then
  for f in save.oneseason.log load.oneseason.log; do
    cat $f | jq . > $f.formatted
  done
  diff -u save.oneseason.log.formatted load.oneseason.log.formatted
  exit 1
fi

# Test that saving and loading a game does not change the simulation results.
cat << EOF > $json_file
{ 'commands' : [
  { 'add_user' : { 'username' : 'test', 'country' : 'faroe_islands', 'team' : 'V. Gotu' }},
  { 'start_bygfoot' : {}},
  { 'simulate_games' : {'weeks' : 5}},
  { 'save_bygfoot' : {'filename' : '$save_dir/save-check-seed.sqlite'}},
  { 'simulate_games' : {'weeks' : 20}},
  { 'dump_bygfoot' : {}},
  { 'simulate_games' : {'weeks' : 40}},
  { 'dump_bygfoot' : {}}
]}
EOF

HOME=$tmphome ./bygfoot --random-seed=1 --json=$json_file > save.log

cat << EOF > $json_file
{ 'commands' : [
  {'load_bygfoot' : {'filename' : '$save_dir/save-check-seed.sqlite'}},
  { 'simulate_games' : {'weeks' : 20}},
  { 'dump_bygfoot' : {}},
  { 'simulate_games' : {'weeks' : 40}},
  { 'dump_bygfoot' : {}}
]}
EOF


HOME=$tmphome ./bygfoot --random-seed=1 --json=$json_file > load.log

if ! diff -q save.log load.log; then
  for f in save.log load.log; do
    cat $f | jq . > $f.formatted
  done
  diff -u save.log.formatted load.log.formatted
  exit 1
fi

rm -R $tmphome

exit 0
