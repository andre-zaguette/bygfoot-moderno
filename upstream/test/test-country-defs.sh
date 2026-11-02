set -e

bygfoot_bin=$1

europe=" \
    albania \
    andorra \
    armenia \
    austria \
    azerbaijan \
    belarus \
    belgium \
    bosnia_herzegovina \
    bulgaria \
    croatia \
    cyprus \
    czech \
    denmark \
    england \
    estonia \
    faroe_islands \
    finland \
    france \
    n_macedonia \
    georgia \
    germany \
    gibraltar \
    greece \
    hungary \
    iceland \
    ireland \
    israel \
    italy \
    kazakhstan \
    kosovo \
    latvia \
    liechtenstein \
    lithuania \
    luxembourg \
    malta \
    moldova \
    montenegro \
    n_ireland \
    netherlands \
    norway \
    poland \
    portugal \
    romania \
    russia \
    san_marino \
    scotland \
    serbia \
    slovakia \
    slovenia \
    spain \
    sweden \
    switzerland \
    turkey \
    ukraine \
    wales"

north_america=" \
    belize \
    costa_rica \
    canada \
    el_salvador \
    guatemala \
    honduras \
    jamaica \
    mexico \
    nicaragua \
    panama \
    puerto_rico \
    trinidad_and_tobago \
    usa \
"

south_america=" \
    argentina \
    bolivia \
	brazil \
	chile \
	colombia \
	ecuador \
	paraguay \
	peru \
	uruguay \
	venezuela \
"

australia=" \
    australia \
    aus_victoria \
"

africa=" \
    algeria \
    angola \
    cameroon \
    congodr \
    egypt \
    ghana \
    ivorycoast \
    mali \
    morocco \
    nigeria \
    sudan \
    tunisia \
    zambia \
    zimbabwe \
"

asia=" \
    australia \
    china \
    india \
    indonesia \
    iran \
    japan \
    korea \
    qatar \
    saudi_arabia \
    singapore \
    thailand \
    uae \
    uzbekistan \
"

countries="$europe $north_america $south_america $africa $australia $asia"

parallel bash $(dirname $0)/test-country-def.sh {} $bygfoot_bin ::: $countries
