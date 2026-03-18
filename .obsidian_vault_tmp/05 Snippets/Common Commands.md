# Common Commands

tags: #snippet

## Git

```bash
git status
git switch -c feature/xxx
git log --oneline --decorate --graph -20
git diff --stat
```

## Docker

```bash
docker ps
docker logs -f <container>
docker exec -it <container> sh
docker compose up -d
docker compose down
```

## macOS

```bash
lsof -i :8080
pbcopy < file.txt
open .
brew services list
```

## Network

```bash
curl -I https://example.com
dig example.com
nc -vz 127.0.0.1 5432
```
