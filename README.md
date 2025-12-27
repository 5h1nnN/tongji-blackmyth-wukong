

# 同济大学软件工程程序设计范式期末项目《黑神话悟空》

## 1	简介

本项目为同济大学计算机科学与技术学院2024级软件工程专业程序设计范式期末项目，基于**虚幻引擎5**，仿照同名游戏，实现简单3D第三人称动作角色扮演游戏。为满足课程对代码与架构设计的考察要求，本项目严格限制蓝图（Blueprint）的使用比例（仅用于资源引用和简单UI），核心逻辑、AI 行为树、战斗判定、角色状态机均使用 C++ 编写。

虚幻引擎版本：`5.4.4`

## 2	学习

1. [虚幻引擎上手](https://www.youtube.com/watch?v=1XjgLKrb4_M)

2. [开始编写游戏](https://dev.epicgames.com/documentation/zh-cn/unreal-engine/code-a-firstperson-adventure-game-in-unreal-engine)

## 3	实现功能

### 3.1 基础功能
| 模块          | 说明                                                       |
| ------------- | ---------------------------------------------------------- |
| **角色系统**  | 悟空模型，实现Idle、移动、奔跑、跳跃、翻滚状态切换         |
| **攻击系统**  | 实现轻棍连招（3段）、重棍                                  |
| **技能系统**  | 棒击震地等（其他见扩展功能）                               |
| **战斗判定**  | 实现了基于Hitbox的攻击范围判定与实时碰撞检测；受击硬直反馈 |
| **敌人 系统** | 3种普通怪物（巡逻、追击）+ 1个 Boss（具二阶段/怒气）       |
| **场景功能**  | 3D 场景漫游、场景切换、场景动态背景音乐                    |
| **UI 系统**   | 标题菜单、玩家/BOSS血条、经验条、技能冷却UI                |

### 3.2 扩展功能
*   变身系统：实现变身为精灵角色。
*   分身系统：生成悟空分身（友方AI系统）和Boss分身。
*   定身系统：通过 C++ 接口暂停目标 AI 的 Behavior Tree 逻辑。
*   道具、掉落物系统：场景中包含可交互或拾取的功能道具（金苹果等）。
*   经验系统：实现属性升级等RPG系统。
*   投射物系统：实现远程攻击

## 4	操控方法
- 无操作 10s 后进入 Idle 状态

### 悟空控制：
- `W`，`A`，`S`，`D` 分别对应前进，向左，向后，向右行走
- `Shift`：加速（奔跑）
- `Ctrl`：跳跃
- `_`（空格）：闪避/翻滚
- 鼠标左键：轻攻击（包含三段连招）
- 鼠标右键：重攻击
- `1`：技能-戳棍法
- `2`：技能-定身术
- `3`：技能-分身术
- `4`：技能-变身术

### UI部分：
- 红色进度条为血条（等级越高血量越多）
- 蓝色进度条为技能冷却时间显示
- 绿色进度条为经验值，其右侧数值为当前等级
- 死亡后有重新开始和退出游戏的选项
- 按 P 键可以暂停或退出游戏

## 5	代码架构与类设计

### 5.1 角色继承体系

利用 C++ 多态机制，设计了通用的角色基类，实现了代码复用。

- `ABaseCharacter` （基类）
  - 继承：继承自`ACharacter`
  - 功能：定义了生命值（`Health`）、受击接口（`TakeDamage`）和死亡虚函数（`Die`）等。
  - 技术点：封装了通用的 `UCharacterMovementComponent` 配置。
- `Ablackmyth_wukongCharacter`（玩家类）
  - 继承：继承自 `ABaseCharacter`
  - 功能：实现了玩家特有的输入绑定（增强输入系统）、轻重棍攻击连招逻辑、闪避及技能。
- `AFeyCharacter`（变身角色类）
  - 继承：继承自 `ABaseCharacter`
  - 功能：实现了变身特有的技能逻辑（如发射 `FeyProjectile`）

### 5.2 AI 与智能体系统

本项目包含两套 AI 系统，分别控制敌人和友方分身。

- `AEnemyAIController`
  - 继承：继承自`AAIController`
  - 功能：驱动普通敌人和 Boss。实现巡逻、发现玩家、追击、攻击及释放技能。
- `ACloneAIController`
  - 继承：继承自`AAIController`
  - 功能：驱动悟空释放“身外身法”产生的分身。分身会继承玩家的当前目标，自动寻找最近的敌对单位进行攻击，并在持续时间结束后自动销毁。

### 5.3 战斗与物理判定系统

摒弃了简单的距离判定，运用动画通知，达到帧级精度的物理碰撞检测。

- `UANS_WeaponCollision` 
  - 继承：继承自 `UAnimNotifyState`
  - 功能：在攻击动作的特定帧区间（挥棍开始到结束）激活碰撞检测。只有在挥动过程中的碰撞才算有效攻击，极大提升了手感。
- `UAnimNotify_FeyAttack`
  - 继承：继承自 `UAnimNotify`
  - 功能：自定义动画通知，用于在动画播放到特定时刻触发 C++ 逻辑。

### 5.4 敌人继承系统

- `AEnemies`
  - 继承：继承自`ACharacter`，`IImobilizableInterface`
  - 功能：为普通怪物的实体类

### 5.5 发射物系统

- `ABaseProjectile`
  - 继承：继承自`AActor`
  - 功能：远程攻击基本逻辑
- `AFeyProjectile`
  - 继承：继承自`ABaseProjectile`
  - 功能：处理`AFeyCharacter`的远程攻击

### 5.6 道具系统

- `ABaseFood`
  - 继承：继承自`AActor`
  - 功能：可拾取道具基类
- `ADoorActor`
  - 继承：继承自`AActor`，`IInteractionInterface`
  - 功能：可交互道具：门

### 5.7 C++接口

- `IInteractionInterface`
  - 功能：实现交互系统
- `IImmobilizableInterface`
  - 功能：实现定身法接口

### 5.8 游戏模式

- `ATitleGameMode`
  - 继承：继承自`AGameModeBase`
  - 功能：实现开始菜单跳转
- `AWukongGameMode`
  - 继承：继承自`AGameModeBase`
  - 功能：实现主游戏关卡逻辑

### 5.9 其他杂项

- `AEnemySpawner`
  - 继承：继承自`AActor`
  - 功能：负责在场景中动态生成怪物波次，使用了 C++ 的 `GetWorld()->SpawnActor` 接口
- `AMyPlayerStart`
  - 继承：继承自`APlayerStart`
  - 功能：设置玩家出生点
- `UMyMainMenuWidget`
  - 继承：继承自`UUserWidget`
  - 功能：实现开始菜单UI逻辑



>  *附：完整提交记录（截至12.27）*

```
*   26713ce (origin/main, origin/HEAD) Merge pull request #14 from 5h1nnN/feature/environment
|\
| * 7a02c3f (origin/feature/environment) 修复跳转关卡时等级经验无法保留的bug
* |   e76178f Merge branch 'feature/enemies'
|\ \
| * | 9e42bde (origin/feature/enemies) 完善Boss战场景 调整Boss_GreatSage视野范围
| * | 190cc44 添加置顶Boss血量显示
| * | 7151506 修复Boss_GreatSage被定身时图标不显示问题
| * | 2e3c176 修复Boss_GreatSage二阶段本体死亡分身未同步死亡问题
| * | 16fe8c1 添加Boss_GreatSage伤害判定及动画
* | |   1a7e57f (HEAD -> main) 完成开始菜单
|\ \ \
| |_|/
|/| |
| * | cf17147 (feature/mainMenu) 完成开始菜单
* | | bb8cbbb 修复行走动画缺失；修复变身后不能暂停；添加分身技能冷却时间 (#13)
|/ /
| | *   097b295 (origin/feature/wukongSkills) Merge branch 'main' into feature/wukongSkills
| | |\
| |_|/
|/| |
* | |   8d8a500 Merge pull request #12 from 5h1nnN/feature/environment
|\ \ \
| |/ /
|/| |
| * | 53b967d 与掉落物苹果之间的交互
* | |   d2912c4 Merge branch 'feature/enemies'
|\ \ \
| * | | e0e06eb 添加Boss_GreatSage受击、死亡、转阶段动画
| * | | f9e32dd 添加敌人Boss_GreatSage
| * | | fea83f2 修改受击判定逻辑
| | | * 2a05a45 修复行走动画缺失；修复变身后不能暂停；添加分身技能冷却时间
| |_|/
|/| |
* | | 6607895 (feature/freeze) 完成定身技能逻辑
| |/
|/|
* |   16e6ac3 Merge pull request #11 from 5h1nnN/feature/environment
|\ \
| |/
|/|
| * 7d845d8 实现关卡跳转功能
* |   1674b2b Merge branch 'feature/enemies'
|\ \
| * | 27b35d2 添加Sparrow远程攻击伤害判定及动画 修复Sparrow僵直问题，现可跟随角色位置旋转视角
| |/
* |   260044c Merge branch 'feature/transform'
|\ \
| |/
|/|
| * 18b53a5 (origin/feature/transform, feature/transform) 完成变身逻辑
| * b7a6405 新建BaseProjectile类
* |   df54d25 Merge branch 'feature/enemies'
|\ \
| * | fa3afca 添加敌人Sparrow 添加Sparrow模型、动画 添加AI控制、受击判定及动画、死亡动画 // 敌人僵直问题待修改、远程伤害判定待添加
| |/
* | 89a7b4a 场景更新
* | d0f464a Change skill keys from 'C' to numbered keys
* | bc653fb 优化操控，添加分身技能 (#10)
* | 906d9a9 角色出生点更新
* | f151b6e 尝试
* |   ffdaa3c 调试
|\ \
| * | befd0b5 场景更新和新关卡引入
| * | 106608c 场景的更新和新关卡的引入
* | | 3fd9be7 新关卡引入
| |/
|/|
* | 01f37ed 修复敌人阵营判定问题 修复Boris血量显示异常问题
* | 04d93c2 Merge branch 'main' into feature/enemies
|\|
| *   9b08e2f Merge branch 'feature/transform'
| |\
| | * b1d6655 完成变身逻辑
| | * 2c93873 Revert "初始化变身逻辑"
| | * d0dbf34 初始化变身逻辑
| | * 99d302b Revert "merge main"
| | * 6d0f49a 创建FeyCharacter类
| | * 9534db7 merge main
| | *   f3ad9e1 merge main
| | |\
| | * | 42a2afa 添加BaseCharacter类
| | * |   cd689ec Merge branch 'main' into feature/transform
| | |\ \
| | * | | daf0e9b 添加TheFey模型
* | | | | cacd9de 添加敌人Boris
* | | | | d43fc45 添加敌人背后受击转身效果 平衡数值
|/ / / /
* | | |   1e2cad0 Merge branch 'feature/enemies'
|\ \ \ \
| * | | | 5e8dfac 添加敌人血量显示 平衡数值
| * | | | 44f3022 增加受击后硬直效果 修复受击、死亡动画显示异常问题
| * | | |   d598eb7 Merge branch 'main' into feature/enemies
| |\ \ \ \
| * | | | | a773563 重写敌人AI控制逻辑
| * | | | | 1eb272a 修复受击、死亡动画无法正常显示问题\n调整武器碰撞体大小，使伤害更易判定
* | | | | |   4a9e0b4 Merge pull request #9 from 5h1nnN/feature/environment
|\ \ \ \ \ \
| |_|_|_|_|/
|/| | | | |
| * | | | | fdc7f56 敌人生成器更新
| * | | | |   4194a22 Merge branch 'main' into feature/environment
| |\ \ \ \ \
| |/ / / / /
|/| | | | |
* | | | | |   110dfd7 Merge branch 'feature/environment'
|\ \ \ \ \ \
| |_|_|/ / /
|/| | | | |
* | | | | |   364b941 Merge branch 'feature/enemies'
|\ \ \ \ \ \
| | |_|/ / /
| |/| | | |
| * | | | | 403fed0 添加Grux武器碰撞体、伤害判定及动画、受击判定及动画、死亡动画
| * | | | | 016c9f7 添加Grux武器碰撞体、伤害判定及动画、受击判定及动画、死亡动画
| * | | | | 8141755 添加首个敌人Grux 添加敌人基础属性设置 绑定Grux骨骼、动画 添加Grux攻击动作 添加敌人AI，实现当悟空进入视野范围内，敌人可识别并进行追踪、攻击
* | | | | | bb288de Update README with control and UI details
| | | * | | a586506 敌人生成落点
| | |/ / /
| | * / / f2ae37b 场景初设置和出生点设置
| |/ / /
|/| | |
* | | | eb8ecfe 添加暂停UI，修复视角移动问题 (#8)
| |_|/
|/| |
* | |   fe1a6a4 完善悟空控制系统
|\ \ \
| |/ /
|/| |
| | | * fa364db (origin/feature/wukongControl) 添加暂停UI，修复视角移动问题
| | |/
| |/|
| * |   100d0f3 Merge branch 'main' into feature/wukongControl
| |\ \
| |/ /
|/| |
* | |   eccf87a 添加经验升级系统，添加Idle
|\ \ \
| * \ \   86d6b2e Merge branch 'main' into feature/wukongControl
| |\ \ \
| |/ / /
|/| | |
* | | | 13cedc9 修改闪避动画，修改死亡动画
| |_|/
|/| |
| | * 7777371 添加戳棍技能，添加收攻动画
| | * b31f22c 添加悟空控制说明文档，添加碰撞检测和攻击范围检测，优化闪避动画
| |/
| * a866b90 添加Idle
| * 986a545 添加升级系统
| * f7241f4 添加跑步功能
| *   ade6d18 Merge branch 'main' into feature/wukongControl
| |\
| |/
|/|
* |   d63fb6d Merge branch 'main' of https://github.com/5h1nnN/tongji-blackmyth-wukong
|\ \
| * | 61df921 添加基础攻击（包括轻击，重击，连招）；添加闪避；添加死亡动画接口
| * |   021cc94 添加攻击动作，添加悟空血条和血量显示
| |\ \
| | * \   75d73d5 Merge branch 'main' into feature/wukongControl
| | |\ \
| | |/ /
| |/| |
| * | | 04a7a10 添加悟空基础移动控制
* | | | 0697583 update readme
|/ / /
| | * 7854b95 更改闪避动画，完善死亡动画
| | * 6982687 添加基础攻击（包括轻击，重击，连招）；添加闪避；添加死亡动画接口
| | * 6ddc45a 添加死亡后重新开始游戏功能
| |/
| * e89ca2f 添加血条和死亡逻辑
| * b3e3cbb 添加攻击动作
| * 514e413 (origin/input) 绑定动画
| * 461a4f1 添加悟空移动控制
| * 2d104c6 添加 Input
|/
* b73dfdc 更换悟空骨骼为ParagonSunWukong
* 7ba61b6 完成悟空骨骼绑定
* 865a6d9 config change
* bed7109 init UE project
* 3cb2d3d update readme
* f457301 gitignore for ue
*   cee4c6d Merge pull request #1 from 5h1nnN/git-config
|\
| * becaff6 (origin/git-config) git配置
| * 12c6f7b git配置
| * 0a6b68c git配置
|/
* 1e49748 init repo
```
