
country=$1
bygfoot_bin=$2
bygfoot_bindir=`dirname $bygfoot_bin`

pushd $bygfoot_bindir
tmphome=`mktemp -d`
json_file="$tmphome/$country.json"
cat <<EOF > $json_file
{ 'commands' : [
  { 'add_country' : { 'name' : '$country' }},
  { 'start_bygfoot' : { }},
  { 'simulate_games' : {'seasons' : 2 }},
]}
EOF

tmphome=`mktemp -d`
if HOME=$tmphome ./bygfoot --random-seed=1 --json=$json_file; then
    echo "$country: PASS"
    status=0
else
    echo "$country: FAIL"
    status=1
fi
rm -Rf $tmphome
exit $status

