FROM python:3.11-slim

RUN apt-get update && apt-get install -y --no-install-recommends \
    gcc graphviz \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY main.c app.py ./

RUN gcc -std=c11 -Wall -Wextra main.c -o main \
    && pip install --no-cache-dir gradio graphviz

CMD ["python", "app.py"]
