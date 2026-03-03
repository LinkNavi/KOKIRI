FROM debian:bookworm-slim
RUN apt-get update && apt-get install -y --no-install-recommends \
    gcc-multilib nasm binutils make \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /os
COPY . .
RUN make clean && make
CMD ["sh", "-c", "echo Build OK && ls -lh kernel.bin"]
