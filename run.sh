#!/bin/bash
cd /app
# rm -f /app/cron.log
echo "Запущен - $(date '+%Y-%m-%d %H:%M:%S')" >> /app/cron.logg
./main config.json

