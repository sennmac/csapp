# Node.js 中的 nextTick、Event Loop 与几个常见调度函数

标签：#backend #snippet

很多人第一次学 Node.js 异步时，会把下面几个东西混在一起：

- `process.nextTick()`
- `queueMicrotask()`
- `setImmediate()`
- `setTimeout(fn, 0)`
- `event loop`

表面看它们都像“稍后执行”，但它们的语义并不一样。真正到线上排障或者写基础库时，这些差异很重要。你要知道当前代码到底是：

- 想立刻在当前调用栈结束后执行
- 想作为微任务尽快执行
- 想明确让出这一轮 event loop
- 还是想真的延迟一段时间

这篇笔记分两部分：

- 先讲清楚 `nextTick` 和 `event loop` 的关系
- 再看几个真实开源库是怎么用这些函数的

## 一、先建立一个正确模型

Node.js 里的 `event loop` 可以粗略理解成一个不断循环的调度器。它会按 phase 推进回调，大致包括：

- `timers`
- `pending callbacks`
- `poll`
- `check`
- `close callbacks`

这里最容易搞混的一点是：`process.nextTick()` 严格说不属于这些 phase。它是 Node.js 在每次当前 JavaScript 执行即将结束时，优先清空的一条特殊队列。

一个足够实用的心智模型是：

```text
当前调用栈
-> process.nextTick 队列
-> 微任务队列（Promise.then / queueMicrotask）
-> event loop 各个 phase
```

也就是说：

- `nextTick` 比普通微任务还激进
- 微任务比 `setImmediate` / `setTimeout` 更早
- `setImmediate` 和 `setTimeout(0)` 都不是“马上执行”，只是进入不同调度位置

## 二、最小例子

```js
console.log('start');

process.nextTick(() => console.log('nextTick'));
Promise.resolve().then(() => console.log('promise'));
setImmediate(() => console.log('immediate'));
setTimeout(() => console.log('timeout'), 0);

console.log('end');
```

常见输出是：

```txt
start
end
nextTick
promise
timeout
immediate
```

这个顺序说明了几件事：

- 当前同步代码永远先执行
- `process.nextTick()` 通常先于 `Promise.then()`
- `setTimeout(0)` 和 `setImmediate()` 的先后不要死记，顶层脚本里不要依赖这个顺序

如果放到 I/O 回调里看：

```js
const fs = require('fs');

fs.readFile(__filename, () => {
  setTimeout(() => console.log('timeout'), 0);
  setImmediate(() => console.log('immediate'));
});
```

常见输出会变成：

```txt
immediate
timeout
```

因为 I/O 回调之后，event loop 更自然地会先推进到 `check` phase，也就是 `setImmediate()` 在的地方。

## 三、`process.nextTick()` 到底适合干什么

它最典型的用途不是“更快的异步”，而是：

- 保证 API 行为一致，不要一会同步一会异步
- 先把内部状态收好，再把事件异步抛给外部
- 避免在当前调用栈里立刻重入某段逻辑

一个很经典的例子是缓存快路径：

```js
function getUser(id, cb) {
  if (cache.has(id)) {
    return process.nextTick(() => cb(null, cache.get(id)));
  }

  db.query(id, cb);
}
```

如果缓存命中直接同步 `cb(...)`，而数据库路径是异步回调，那调用方就很容易踩坑。用 `nextTick` 的目的就是把同步快路径也变成异步。

但 `nextTick` 不能滥用。因为它优先级太高，如果递归塞任务，I/O 会被饿死：

```js
function spin() {
  process.nextTick(spin);
}

spin();
```

这段代码可能不会立刻报错，但它会让 event loop 长时间回不到 I/O、timer、`setImmediate` 那些 phase。

## 四、开源库里是怎么用这些函数的

下面看几个真实仓库里的例子。重点不是记源码，而是理解作者为什么选这个函数。

## 五、例子一：`readable-stream` 用 `process.nextTick()` 保证事件顺序

源码位置：

- `nodejs/readable-stream`
- `lib/internal/streams/destroy.js`

这个库在流销毁时，不会同步触发 `error` 和 `close`，而是先修改内部状态，再通过 `process.nextTick()` 发事件。逻辑可以简化成这样：

```js
function onDestroy(err) {
  state.destroyed = true;
  state.closed = true;

  if (err) {
    process.nextTick(() => {
      emitError(err);
      emitClose();
    });
  } else {
    process.nextTick(() => {
      emitClose();
    });
  }
}
```

这里为什么不用同步 `emit`？

- 监听器拿到对象时，`destroyed`、`closed` 这些状态已经一致
- 防止监听器内部再次调用 `destroy()` 导致重入问题
- 保证 `error` / `close` 的触发时机更稳定，不会混在当前调用栈里

所以这个场景下，`nextTick` 是“事件顺序控制工具”，不是“性能优化工具”。

## 六、例子二：`async` 用 defer 包装避免回调一会同步一会异步

源码位置：

- `caolan/async`
- `lib/internal/setImmediate.js`
- `lib/ensureAsync.js`

`async` 库内部做了一层统一 defer，大意是：

```js
const defer =
  queueMicrotask ??
  setImmediate ??
  process.nextTick ??
  ((fn) => setTimeout(fn, 0));
```

然后在 `ensureAsync` 里，如果发现某个函数同步调用了回调，就强制 defer 一次：

```js
function ensureAsync(fn) {
  return (...args) => {
    const callback = args.pop();
    let sync = true;

    args.push((...result) => {
      if (sync) {
        defer(() => callback(...result));
      } else {
        callback(...result);
      }
    });

    fn(...args);
    sync = false;
  };
}
```

它要解决的问题很典型：

```js
function sometimesAsync(key, cb) {
  if (cache[key]) return cb(null, cache[key]);
  doSomeIO(key, cb);
}
```

这个函数在缓存命中时是同步，在 I/O 路径上是异步。调用方如果串很多层，很容易出现栈过深、时序不一致、难排查的 bug。

`async.ensureAsync()` 的核心价值就是把这种行为统一掉。

这里值得注意的一点是：它没有盲目把 `process.nextTick()` 当默认首选，而是优先更温和的 defer 方式。这说明对通用库作者来说，`nextTick` 的优先级常常太高。

## 七、例子三：`p-limit` 用 `queueMicrotask()` 延后并发队列刷新

源码位置：

- `sindresorhus/p-limit`
- `index.js`

`p-limit` 是并发限制库。它在动态修改 `concurrency` 时，不会立刻同步刷队列，而是先放进 `queueMicrotask()`：

```js
set(newConcurrency) {
  concurrency = newConcurrency;

  queueMicrotask(() => {
    while (activeCount < concurrency && queue.size > 0) {
      resumeNext();
    }
  });
}
```

作者为什么这么写？

- 当前 setter 里的同步状态变更要先完成
- 不想立刻重入队列调度逻辑
- 也不想等到下一轮 timer 或 I/O phase

所以这里最合适的是微任务：当前同步逻辑结束后，马上在本轮尾部把一致性修正掉。

这个例子很适合理解 `queueMicrotask()` 的定位：

- 比 `setImmediate` 更早
- 比 `setTimeout` 更近
- 但比直接同步执行更安全

## 八、例子四：`p-retry` 用 `setTimeout()` 做真实的退避等待

源码位置：

- `sindresorhus/p-retry`
- `index.js`

这个库在失败后会计算下一次重试间隔，然后通过 `setTimeout()` 真正等待：

```js
await new Promise((resolve, reject) => {
  const timer = setTimeout(resolve, delay);

  signal?.addEventListener('abort', () => {
    clearTimeout(timer);
    reject(signal.reason);
  }, {once: true});
});
```

这里它选择 `setTimeout()` 的原因就很纯粹：

- 不是为了调度顺序
- 不是为了本轮尾部执行
- 就是为了“过 N 毫秒再重试”

这个场景常见于：

- retry/backoff
- debounce
- timeout
- 轮询

这个库还有一个很实用的细节：如果开启对应选项，会对 timer 调 `unref()`。意思是如果进程里只剩这个定时器，不要因为它还在等待就把整个进程硬拖住。

## 九、例子五：某些队列库会用 `setImmediate()` 主动让出这一轮 event loop

有些队列或任务调度库，在处理完一个任务后不会立刻同步触发下一个，而是显式用 `setImmediate()`：

```js
function triggerNext() {
  if (queue.length > 0) {
    setImmediate(triggerNext);
  }
}
```

这么做的意图通常是：

- 避免一口气同步跑完整个队列
- 给 I/O、timer、close callback 等其他阶段留执行机会
- 降低长链任务对 event loop 的独占

所以 `setImmediate()` 更像一种“我知道后面还有活，但先把这一轮让出去”的调度姿态。

## 十、怎么选这几个函数

如果你是在业务代码里选，最实用的判断表是：

- `process.nextTick()`：需要 Node 特有语义，当前栈结束后立刻执行，常用于修正状态后发事件
- `queueMicrotask()`：需要本轮尾部尽快执行，但不想上升到 `nextTick` 那么激进
- `setImmediate()`：想明确让出这一轮 event loop，再继续做事
- `setTimeout()`：想表达真实时间延迟

换一种更工程化的说法：

- “我要保持 API 异步一致性” -> 优先考虑 `queueMicrotask()` 或经过设计的 defer
- “我要保证内部状态先稳定，再通知外部” -> `process.nextTick()`
- “我要避免饿死 I/O，主动往后放一放” -> `setImmediate()`
- “我要过 200ms 再来一次” -> `setTimeout()`

## 十一、最容易踩的坑

### 1. 把 `nextTick` 当成通用异步工具

这是最常见误用。它优先级太高，递归调度时很容易饿死 I/O。

### 2. 误以为 `setTimeout(fn, 0)` 等于马上执行

不是。它只是说最早在 timer 阶段触发，不保证“立即”。

### 3. 在顶层脚本里强依赖 `setImmediate()` 和 `setTimeout(0)` 的先后

这个顺序不应该被拿来写逻辑依赖，尤其不要把它当 API 契约。

### 4. API 有时同步有时异步

这类问题在基础库里尤其危险。很多时候你不是需要“更快”，而是需要“行为始终一致”。

## 十二、一个足够实用的结论

如果你只是写普通业务，不要先想到 `process.nextTick()`。多数场景下：

- 微任务用 `queueMicrotask()` 或 `Promise.then()`
- 明确让出 event loop 用 `setImmediate()`
- 真实延迟用 `setTimeout()`

只有当你非常明确自己在处理 Node 风格的内部时序问题，比如“状态先更新，再发事件”、“保持 Node API 的回调语义一致”时，`process.nextTick()` 才是正确工具。

## 参考源码

- `readable-stream`: <https://github.com/nodejs/readable-stream/blob/main/lib/internal/streams/destroy.js>
- `async`: <https://github.com/caolan/async/blob/master/lib/internal/setImmediate.js>
- `async`: <https://github.com/caolan/async/blob/master/lib/ensureAsync.js>
- `p-limit`: <https://github.com/sindresorhus/p-limit/blob/main/index.js>
- `p-retry`: <https://github.com/sindresorhus/p-retry/blob/main/index.js>
