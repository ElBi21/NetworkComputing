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
    docker exec -ti --privileged netcomp_server bash

[doc("Connect to the first client container")]
client_1_conn:
    docker exec -ti --privileged netcomp_client_1 bash

[doc("Connect to the second client container")]
client_2_conn:
    docker exec -ti --privileged netcomp_client_2 bash



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

[doc("Clean the containers")]
clean:
    docker rm -f netcomp_server
    docker rm -f netcomp_client_1
    docker rm -f netcomp_client_2
    docker image rm networkcomputing-exp_server:latest
    docker image rm networkcomputing-exp_client_1:latest
    docker image rm networkcomputing-exp_client_2:latest