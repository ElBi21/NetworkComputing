[doc("Build the Docker network and containers")]
build:
    docker compose -p networkcomputing up -d

[doc("Connect to the server container")]
server_conn:
    docker exec -it netcomp_server bash

[doc("Connect to the first client container")]
client_1_conn:
    docker exec -it netcomp_client_1 bash

[doc("Connect to the second client container")]
client_2_conn:
    docker exec -it netcomp_client_2 bash



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