# API 文档站点说明

本目录包含 PicoCamera 的 API 在线文档配置，使用 **Doxygen** 生成、
**GitHub Pages** 托管、**Giscus**（GitHub Discussions）提供评论区。

站点提供双语版本，每页页脚可互相切换：

- English：<https://umeiko.github.io/PicoCamera/>（主页为 `readme.md`）
- 中文：<https://umeiko.github.io/PicoCamera/zh/>（主页为 `readme_cn.md`，界面语言为中文）

## 自动部署（CI）

`.github/workflows/docs.yml` 在以下情况自动构建并部署：

- `main` 分支上 `src/`、`docs/`、`readme.md` 等文件有 push
- 在 Actions 页面手动触发（workflow_dispatch）

doxygen-awesome 主题 CSS（v2.3.4）已随仓库提交（`docs/doxygen-awesome*.css`），
构建产物目录 `docs/_build/` 已被 `.gitignore` 忽略。

## 一次性配置

Giscus 所需的前期准备（开启 Discussions、安装 Giscus 应用、填入
`data-repo-id` / `data-category-id`）**均已完成**，`docs/footer.html` 中的
分类为 `Announcements`。

剩余唯一步骤：**仓库 Settings → Pages → Build and deployment → Source 选
GitHub Actions**。之后 push 到 `main`，Actions 跑完即可访问站点，每个文档
页面底部都会出现评论区。

如需调整评论区行为（分类、主题、语言等），直接改 `docs/footer.html` 中的
`data-*` 属性即可，可选项见 <https://giscus.app/zh-CN>。

## 本地构建（可选）

需要安装 [Doxygen](https://www.doxygen.nl/download.html)（可选 graphviz 用于继承图）。
**注意**：`docs/header.html` 基于 Doxygen 1.18.0 的模板定制，本地构建请使用
**1.18.0 或更高版本**，旧版本会导致模板变量不替换、侧边导航栏失效（CI 已固定
使用 1.18.0 官方二进制）。

```bash
# 构建（必须在 docs/ 目录下运行，主题 CSS 已随仓库提交）
cd docs
doxygen Doxyfile       # 英文版 → _build/html/
doxygen Doxyfile_zh    # 中文版 → _build/html/zh/
# 输出在 docs/_build/html/index.html
```

注意：Giscus 评论区只在线上域名（`umeiko.github.io`）下正常加载，本地打开页面时评论区不显示属正常现象。

## 文件清单

| 文件 | 作用 |
|------|------|
| `docs/Doxyfile` | Doxygen 配置（输入为 `src/` 头文件 + `readme.md` 主页） |
| `docs/Doxyfile_zh` | 中文版配置（`@INCLUDE` 基础配置后覆盖主页、语言，输出到 `html/zh/`） |
| `docs/doxygen-awesome*.css` | 页面主题（v2.3.4，随仓库提交） |
| `docs/custom.css` | 自定义样式覆盖（修复深色模式下行内代码白底问题） |
| `docs/user_guide.md` / `docs/user_guide_zh.md` | 用户指南（英文/中文），作为页面收录进对应语言站点 |
| `docs/footer.html` / `docs/footer_zh.html` | 每页页脚：语言切换链接 + Giscus 评论脚本 |
| `docs/header.html` | 每页页眉（自定义，右上角 GitHub 仓库链接） |
| `.github/workflows/docs.yml` | 构建 + 部署到 GitHub Pages |
| `docs/_build/` | 构建产物（不提交） |
