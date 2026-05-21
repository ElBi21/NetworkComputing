<h1 align="center">Network Computing Project 🌐</h1>

Project for recreating the BMC paper as part of the Network Computing course project

## Docker Setup for local development

0. Make sure that:
    - you have Docker installed on your machine;
    - you're not using ports `9010`, `9011` and `9012`;

1. After having unpacked, run:

```sh
docker compose -p networkcomputing up -d
```

(if you have [Just](https://just.systems/man/en/) installed, run `just build` instead)

2. Wait for the build process to end, and then you will be able to connect to the containers with

```sh
docker exec -it <container_name> bash
```

(if you have [Just](https://just.systems/man/en/) installed, run `just server_conn|client_1_conn|client_2_conn` instead)

The possible values for `<container_name>` are:

- `netcomp_server`: the server with BMC;
- `netcomp_client_1` and `netcomp_client_2`: the first and second clients.

> [!WARNING]
> **Be careful**: the containers will run indefinitely in the background. Make sure to stop them wither with `docker stop <container_name>` or through a tool like [Portainer](https://docs.portainer.io/start/install-ce). They can be started again either through Portainer or with `docker start <container_name>`
>
> Again, if you have [Just](https://just.systems/man/en/) installed, you can run `just run` and `just stop` instead

## Network description

The containers are on two subnets:

- `10.0.1.0/24` for the internal communication;
- `10.0.2.0/24` for the external communication.

Specifically, the 4th digit of the IP address used is `2` for the server, `3` for client 1 and `4` for client 2. For instance, the internal IP address for the 2nd client is `10.0.1.4`, while its external one is `10.0.2.4`. You can look at all the addresses in the docker-compose.yml file.
