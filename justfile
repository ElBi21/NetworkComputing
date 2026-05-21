KEYS := "50000"
OPS := "500000"
THREADS := "8"
ALPHA := "1.2"
RATIO := "0.8"
VALUE_SIZE := "256"



[doc("[ONLY CLIENT 1] Stress test the server")]
run_stress_1:
    uv run memcached_stress.py --num-keys {{KEYS}} --num-ops {{OPS}} --threads {{THREADS}} --alpha {{ALPHA}} --get-ratio {{RATIO}} --value-size {{VALUE_SIZE}} --warm-up

[doc("[ONLY CLIENT 2] Stress test the server")]
run_stress_2:
    uv run memcached_stress.py --num-keys {{KEYS}} --num-ops {{OPS}} --threads {{THREADS}} --alpha {{ALPHA}} --get-ratio {{RATIO}} --value-size {{VALUE_SIZE}}


# ======== Building & Setup commands ========

[doc("Build the Docker network and containers")]
build:
    docker compose -p networkcomputing up -d



# ======== Containers commands ========

[doc("Connect to the server container")]
server_conn:
    docker exec --privileged -it netcomp_server bash

[doc("Connect to the first client container")]
client_1_conn:
    docker exec -it netcomp_client_1 bash

[doc("Connect to the second client container")]
client_2_conn:
    docker exec -it netcomp_client_2 bash



# ======== Docker commands ========

[doc("Start the containers")]
start:
    docker start netcomp_server
    docker start netcomp_client_1
    docker start netcomp_client_2

[doc("Stop the containers")]
stop:
    docker stop netcomp_server
    docker stop netcomp_client_1
    docker stop netcomp_client_2