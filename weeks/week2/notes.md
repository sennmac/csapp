# Week 2 Notes

- Day 1:
  - 做了什么：运行 `./bench` 对 `N=512/1024/2048` 做 row-major vs col-major 基线对比。
  - 数据摘要：
    - `N=512`: row `1.17 ns/elem`，col `1.14 ns/elem`（差异很小）
    - `N=1024`: row `1.07 ns/elem`，col `1.06 ns/elem`（差异很小）
    - `N=2048`: row `0.94 ns/elem`，col `2.17 ns/elem`（col 明显变慢，约 2.3x）
  - 解释：矩阵变大后，col-major 访问步长为 `N`，空间局部性变差，cache line 利用率下降；row-major 连续访问，局部性更好。
  - 结论：当数据规模超过缓存友好范围时，访问顺序会成为主导性能因素。
  - 明日计划：进入 Day 2，使用 `cache_sim` 跑 trace，验证 hit/miss/eviction 与访问模式之间的关系。
- Day 2:
- Day 3:
- Day 4:
- Day 5:
