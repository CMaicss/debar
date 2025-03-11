# debar

与环境无关的 apt 仓库客户端。

此软件目标是能在任意发行版和架构下，获取任何 debian 及其衍生版的仓库数据，并且支持下载任意软件包并自动获取其全部依赖。适用于需要在离线环境中安装软件时快速获取最小包的集合。

## 命令

```
Download deb package with his depend from debian repository.
Usage:
  debar [OPTION...]

      --init                Init repo data in current directory.
      --update              Update repo data in current directory.
      --get <package_name>  Download deb package and depends.
      --help                Print help.
```

## 用法

**1. 初始化工作目录**

新建一个空目录，并初始化。

```sh
mkdir debwork
cd debwork
debar --init
```

**2. 编辑配置文件**

初始化完成后会在当前目录创建一个配置文件：config.yaml，如下：

```yaml
repo:
  url: http://security.ubuntu.com/ubuntu/
  components:
    - main
    - restricted
    - universe
    - multiverse
  arch: amd64
  release_name: noble
```

您需要编辑此配置文件，将您想要访问的仓库信息填写在此文件中。

一般来说，您可以参考 debain 及其衍生版的 `/etc/apt/sources.list` 文件。如 ubuntu 下，此文件包含：

```
deb http://security.ubuntu.com/ubuntu/ noble main restricted universe multiverse
```

这行内容与 `config.yaml` 中这些字段的对应关系显而易见。

架构字段取决于发行版的仓库是如何组织的，并确定是否支持你的目标架构。要查看支持的全部列表，可从上面的 url 对应的仓库下取得，例如浏览器访问以下 url：

```url
http://security.ubuntu.com/ubuntu/dists/bionic/main
```

顺利的话你可以在浏览器中看到若干以 `binary-` 开头的文件夹，其名称的后半部分就是架构，若存在则支持。


**3. 更新缓存**

配置文件编写完成后需要将包的列表数据下载下来，这时需要使用：

```sh
debar --update
```

**4. 下载软件包**

一切准备就绪后即可下载任意软件包：

```sh
debar --get vim
```

上述操作会将 vim 及其全部依赖（包括间接依赖）全部下载到当前目录，而您只需要来一杯咖啡，静静等待。

## 里程碑

|功能|说明|状态|
|---|----|----|
|下载包|下载包及其全部依赖|✅|
|搜索包|按名称或说明搜索包|🐢|
|查询依赖关系|查询一个包的全部依赖关系|🐢|
|查询软件包基本信息|查询一个包的基本信息|🐢|
|支持配置排除列表|支持在下载软件包时忽略一部分依赖包|🐢|
