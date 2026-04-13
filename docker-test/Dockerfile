FROM --platform=$BUILDPLATFORM gcc:latest AS builder

ARG TARGETPLATFORM

WORKDIR /usr/src/app

COPY hello.c .

RUN if [ "$TARGETPLATFORM" = "linux/arm64" ]; then \
	apt-get update && apt-get install -y gcc-aarch64-linux-gnu && \
	aarch64-linux-gnu-gcc -O0 -g hello.c -o hello; \
	else \
	gcc -O0 -g hello.c -o hello; \
	fi

FROM debian:stable-slim

WORKDIR /usr/bin

COPY --from=builder /usr/src/app/hello hello

CMD ["./hello"]
