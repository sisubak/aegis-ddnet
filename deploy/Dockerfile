# Coded by ueha
# Многостадийный runtime-образ сервера aegis-ddnet (gate + game).
# Сборка (контекст = корень репозитория):
#   docker build -f deploy/Dockerfile -t aegis-server .
# Рецепт сборки повторяет зелёный CI release-servers.yml (ubuntu, CLIENT=OFF).

FROM debian:12 AS build
RUN apt-get update && apt-get install -y --no-install-recommends \
      build-essential cmake ninja-build pkg-config git ca-certificates curl \
      libsqlite3-dev libssl-dev libcurl4-openssl-dev zlib1g-dev \
 && rm -rf /var/lib/apt/lists/*
RUN curl https://sh.rustup.rs -sSf | sh -s -- --default-toolchain stable -y
ENV PATH="/root/.cargo/bin:${PATH}"
WORKDIR /src
COPY . /src
RUN cmake -S /src -B /src/build -G Ninja \
      -DCMAKE_BUILD_TYPE=Release -DCLIENT=OFF -DTOOLS=OFF -DDOWNLOAD_GTEST=OFF \
      -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=/src/build \
 && cmake --build /src/build --target game-server

FROM debian:12-slim AS runtime
RUN apt-get update && apt-get install -y --no-install-recommends \
      libsqlite3-0 libssl3 libcurl4 zlib1g ca-certificates \
 && rm -rf /var/lib/apt/lists/*
WORKDIR /aegis
COPY --from=build /src/build/DDNet-Server /aegis/DDNet-Server
COPY --from=build /src/data /aegis/data
COPY deploy/storage.cfg deploy/server.cfg deploy/gate.cfg /aegis/
EXPOSE 8303/udp 8304/udp
ENTRYPOINT ["/aegis/DDNet-Server"]
CMD ["exec server.cfg"]
