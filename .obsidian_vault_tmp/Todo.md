# Todo

这是全局任务页。

## All Open Tasks

```dataview
TASK
FROM ""
WHERE !completed
SORT file.mtime DESC
```

## Projects Only

```dataview
TASK
FROM "02 Projects"
WHERE !completed
SORT file.name ASC
```

## Daily Notes Only

```dataview
TASK
FROM "01 Daily"
WHERE !completed
SORT file.name DESC
```

## How To Use

在任意笔记里直接写任务：

```md
- [ ] 修复登录 bug
- [ ] 整理 Redis 笔记
- [ ] 给支付服务加超时控制
```

这里会自动汇总未完成项。
