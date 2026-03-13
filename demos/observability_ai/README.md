# Dynatrace + Grafana AI Data Access Templates

这套脚本用于“先拿到观测数据，再交给 AI 做总结/问答”。

## 1) Dynatrace: Metrics API v2

环境变量：

```bash
export DYNATRACE_BASE_URL="https://<your-env>.live.dynatrace.com"
export DYNATRACE_API_TOKEN="<dt_api_token_with_metrics_read>"
```

示例：

```bash
python3 dynatrace_metrics_query.py \
  --metric-selector "builtin:host.cpu.usage" \
  --from "now-2h" \
  --to "now" \
  --resolution "1m" \
  --output dt_cpu.json
```

输出是原始 JSON，可直接喂给你的 AI 流水线做分析。

## 2) Grafana: 读取 Dashboard JSON

环境变量：

```bash
export GRAFANA_URL="https://<grafana-host>"
export GRAFANA_TOKEN="<service_account_token>"
```

示例：

```bash
python3 grafana_get_dashboard.py \
  --uid "<dashboard_uid>" \
  --output dashboard.json
```

## 3) Grafana: 调 Data Source Query API

1) 先准备 `payload.json`（可从 Panel/Query Inspector 复制请求体）  
2) 再执行：

```bash
python3 grafana_ds_query.py \
  --payload payload.json \
  --output grafana_query_result.json
```

## AI 接入建议

标准流程：
1. 脚本拉原始 JSON
2. 送入 AI（比如总结异常、环比/同比、Top N 波动）
3. 把 AI 输出写入日报或告警解释

注意：
- 不要把 token 写死到代码里
- 生产环境先做最小权限 token
- 对输出做脱敏（主机名/IP/用户标识）
