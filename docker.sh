#!/bin/bash
# Script to start and enter the TPE-ARQ Docker container

CONTAINER_NAME="TPE-SO-2-g24-65627-65145-65202"

# COLORS
RED='\033[0;31m'
YELLOW='\033[1;33m'
GREEN='\033[0;32m'
NC='\033[0m'

# Check if Docker is running
docker ps -a &> /dev/null

if [ $? -ne 0 ]; then
    echo -e "${RED}Docker is not running. Please start Docker and try again.${NC}"
    exit 1
fi

# Check if container exists
if [ ! "$(docker ps -a | grep "$CONTAINER_NAME")" ]; then
    echo -e "${YELLOW}Container $CONTAINER_NAME does not exist.${NC}"
    echo "Pulling image..."
    docker pull agodio/itba-so-multi-platform:3.0
    echo "Creating container..."
    docker run -d -v ${PWD}:/root --security-opt seccomp:unconfined -it --name "$CONTAINER_NAME" agodio/itba-so-multi-platform:3.0
    echo -e "${GREEN}Container $CONTAINER_NAME created.${NC}"
else
    echo -e "${GREEN}Container $CONTAINER_NAME exists.${NC}"
fi

# Start container if not running
if [ ! "$(docker ps | grep "$CONTAINER_NAME")" ]; then
    docker start "$CONTAINER_NAME" &> /dev/null
    echo -e "${GREEN}Container $CONTAINER_NAME started.${NC}"
else
    echo -e "${GREEN}Container $CONTAINER_NAME is already running.${NC}"
fi

# Enter the container
echo -e "${GREEN}Entering container $CONTAINER_NAME...${NC}"
docker exec -it "$CONTAINER_NAME" /bin/bash
