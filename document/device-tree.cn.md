# Device Trees, overlays, and parameters

Raspberry Pi的最新内核和固件（包括Raspbian和NOOBS版本）现在默认使用设备树（DT）来管理一些资源分配和模块加载。这样做是为了缓解多个驱动程序争用系统资源的问题，并允许HAT模块自动配置。

当前的实现不是纯粹的设备树系统-仍然存在创建某些平台设备的板级支持代码-但是外部接口（I2C，I2S，SPI）以及使用它们的音频设备现在必须使用设备实例化加载程序（start.elf）将Tree Blob（DTB）传递到内核。

除非DTB要求，否则使用设备树的主要影响是将所有内容（依靠模块黑名单来管理争用）更改为全部关闭。为了继续使用外部接口和与其连接的外围设备，您将需要在config.txt中添加一些新设置。第3部分中提供了完整的详细信息，但以下是一些示例：

```sh
# 取消注释部分或全部这些行以启用可选硬件接口
#dtparam=i2c_arm=on
#dtparam=i2s=on
#dtparam=spi=on

# 取消注释这些行之一以启用音频接口
#dtoverlay=hifiberry-amp
#dtoverlay=hifiberry-dac
#dtoverlay=hifiberry-dacplus
#dtoverlay=hifiberry-digi
#dtoverlay=iqaudio-dac
#dtoverlay=iqaudio-dacplus
#dtoverlay=audioinjector-wm8731-audio

# 取消注释此选项以启用lirc-rpi模块
#dtoverlay=lirc-rpi

# 取消注释此选项以覆盖lirc-rpi模块的默认值
#dtparam=gpio_out_pin=16
#dtparam=gpio_in_pin=17
#dtparam=gpio_in_pull=down
```

## Device Trees

设备树（DT）是对系统中硬件的描述。 它应包括基本CPU的名称，其内存配置以及所有外围设备（片内的和片外的）。 DT不应用于描述软件，尽管通过列出硬件模块通常会导致加载驱动程序模块。 记住DT应该是与操作系统无关的，这可能有助于记住，所以任何Linux特定的内容都不应该存在。

设备树将硬件配置表示为节点的层次结构。 每个节点都可以包含属性和子节点。 属性被命名为字节数组，可以包含字符串，数字（大端），任意字节序列及其任意组合。 类似于文件系统，节点是目录，属性是文件。 树中节点和属性的位置可以使用路径来描述，以斜杠作为分隔符，并使用单个斜杠（`/`）表示根。

### 1.1: DTS基础语法

设备树通常以称为设备树源（DTS）的文本形式编写，并存储在带有`.dts`后缀的文件中。 DTS语法类似于C，每行的末尾都有用于分组的括号和分号。 请注意，DTS在右大括号后需要分号：相对于C的`struct`而不是函数。 编译后的二进制格式称为“扁平化设备树（FDT）”或“设备树Blob（DTB）”，并存储在`.dtb`文件中。

以下是.dts格式的简单树：

```sh
/dts-v1/;
/include/"common.dtsi";

/{
    node1 {
        a-string-property = "A string";
        a-string-list-property = "first string", "second string";
        a-byte-data-property = [0x01 0x23 0x34 0x56];
        cousin: child-node1 {
            first-child-property;
            second-child-property = <1>;
            a-string-property = "Hello, world";
        };
        child-node2 {
        };
    };
    node2 {
        an-empty-property;
        a-cell-property = <1 2 3 4>; /* each number (cell) is a uint32 */
        child-node1 {
            my-cousin = <&cousin>;
        };
    };
};

/node2 {
    another-property-for-node2;
};
```

该树包含：

- 必需的标头：`/dts-v1/`。
- 包含另一个DTS文件，该文件通常名为`*.dtsi`，类似于C中的.h头文件-请参见下面的/include/。
- 单个根节点：`/`
- 几个子节点：`node1`和`node2`
- node1的一些子节点：`child-node1`和`child-node2`
- 标签（`cousin`）和对该标签的引用（`&cousin`）：请参阅下面的标签和引用。
- 分散在树上的几个属性
- 一个重复的节点（`/node2`）-参见下面的/include/。

属性是简单的键/值对，其中值可以为空或包含任意字节流。 尽管数据类型未在数据结构中编码，但是可以在设备树源文件中表达一些基本数据表示形式。

文本字符串（NUL终止）用双引号表示：

```sh
string-property = "a string";
```

单元格是由尖括号分隔的32位无符号整数：

```sh
cell-property = <0xbeef 123 0xabcd1234>;
```

任意字节数据用方括号定界，并以十六进制输入：

```
binary-property = [01 23 45 67 89 ab cd ef];
```

可以使用逗号将不同表示形式的数据连接起来：

```sh
mixed-property = "a string", [01 23 45 67], <0x12345678>;
```

逗号还用于创建字符串列表：

```sh
string-list = "red fish", "blue fish";
```

### 1.2: 关于/include/

`/include/`指令是简单的文本包含，就像C的`#include`指令一样，但是Device Tree编译器的功能导致了不同的使用模式。 给定节点的名称（可能带有绝对路径），则同一个节点可能在DTS文件（及其包含文件）中出现两次。 发生这种情况时，将节点和属性组合在一起，根据需要进行交织和覆盖属性（后面的值会覆盖前面的值）。

在上面的示例中，`/node2`的第二次出现导致将新属性添加到原始属性：

```sh
/node2 {
    an-empty-property;
    a-cell-property = <1 2 3 4>; /* each number (cell) is a uint32 */
    another-property-for-node2;
    child-node1 {
        my-cousin = <&cousin>;
    };
};
```

因此，一个`.dtsi`可能会覆盖树中的多个位置或为其提供默认值。

### 1.3: 标签与引用

树往往会一部分引用另一部分，有四种方法可以做到这一点：

1. 路径字符串

类似于文件系统，路径应该是不言自明的 - `/soc/i2s@7e203000`是BCM2835和BCM2836中I2S设备的完整路径。请注意，尽管构造属性的路径很容易（例如，`/soc/i2s@7e203000/status`），但是标准API却不这样做。您首先找到一个节点，然后选择该节点的属性。

2. phandles(把手)

把手是在其`phandle`属性中分配给节点的唯一32位整数。由于历史原因，您可能还会看到一个冗余的，匹配`linux，phandle`。把手从1开始顺序编号； 0不是有效的。当它们在整数上下文中遇到对节点的引用时，通常由DT编译器分配它们，通常以标签的形式（请参见下文）。使用句柄对节点的引用被简单地编码为相应的整数（单元）值。没有标记指示应将它们解释为phandles，因为这是应用程序定义的。

3. Labels(标签)

就像C中的标签为代码中的位置提供名称一样，DT标签也为层次结构中的节点分配名称。编译器获取对标签的引用，并将其在字符串上下文（`＆node`）中使用时转换为路径，在整数上下文（`<＆node>`）中使用phandles；原始标签不会出现在编译输出中。请注意，标签不包含任何结构。它们只是一个统一的全局命名空间中的令牌。

4. Aliases(别名)

别名与标签类似，不同的是别名确实以索引形式出现在FDT输出中。它们存储为`/aliases`节点的属性，每个属性都将别名映射到路径字符串。尽管别名节点出现在源中，但路径字符串通常显示为对标签（`＆node`）的引用，而不是全部写出。 DT API将节点的路径字符串解析为路径的首字符，通常将不以斜杠开头的路径视为别名，必须先使用`/aliases`表将其转换为路径。

### 1.4：设备树语义

如何构造设备树，以及如何最好地使用它来捕获某些硬件的配置是一个大而复杂的主题。有很多可用的资源，下面列出了其中的一些资源，但是在本文档中有几点需要提及：

`compatible`(兼容性)是硬件描述和驱动程序软件之间的链接。当操作系统遇到具有`compatible`属性的节点时，它将在其设备驱动程序数据库中查找该节点以找到最佳匹配项。在Linux中，如果已正确标记驱动程序模块且未将其列入黑名单，这通常会导致驱动程序模块自动加载。

`status`(状态)属性指示设备是启用还是禁用。如果`status`为`ok`、`okay`或不存在，则表示设备已启用。否则`status`应为`disabled`，以便禁用设备。将设备置于状态设置为`disabled`的`.dtsi`文件中可能很有用。然后，派生的配置可以包括该`.dtsi`并设置需要的设备状态。

## 第2部分：设备树覆盖

现代的SoC（片上系统）是非常复杂的设备。完整的设备树可能长达数百行。再往前走一步，将SoC与其他组件一起放置在板上，只会使情况变得更糟。为了保持可管理性，特别是在存在共享组件的相关设备的情况下，将公共元素放入`.dtsi`文件中是很有意义的，以便可能包含多个`.dts`文件。

但是，当像Raspberry Pi这样的系统支持可选的插件附件（例如HAT）时，问题就会加剧。最终，每种可能的配置都需要一个设备树来描述它，但是一旦您考虑到不同的基本硬件（模型A，B，A +和B +），并且小工具只需要使用几个可以共存的GPIO引脚，组合开始迅速繁殖。

所需要的是一种使用部分设备树描述这些可选组件的方法，然后通过采用基本DT并添加许多可选元素来构建完整的树。您可以执行此操作，这些可选元素称为“叠加层”。

### 2.1：Fragments(片段)

DT覆盖包括多个片段，每个片段都针对一个节点及其子节点。尽管这个概念听起来很简单，但是语法一开始似乎很奇怪：

```sh
//Enable the i2s interface
/dts-v1/;
/plugin/;

/{
    compatible = "brcm,bcm2708";

    fragment@0 {
        target = <&i2s>;
        __overlay__ {
            status = "okay";
        };
    };
};
```

`compatible`字符串将其标识为BCM2708，这是BCM2835部件的基本体系结构。 对于BCM2836部分，您可以使用兼容的字符串“ brcm，bcm2709”，但是除非您针对的是ARM CPU的功能，否则两种架构应该等效，因此坚持使用“ brcm，bcm2708”是合理的。 然后是第一个（仅在这种情况下）片段。 片段从零开始顺序编号。 不遵守此规定可能会导致丢失部分或全部片段。

每个片段由两部分组成：一个`target`属性，标识要应用叠加层的节点； 和`__overlay__`本身，其主体将添加到目标节点。 可以将上面的示例解释为如下所示：

```
/dts-v1/;

/ {
    compatible = "brcm,bcm2708";
};

&i2s {
    status = "okay";
};
```

如果覆盖层是在以后加载的，那么将覆盖层与标准的Raspberry Pi基本设备树（例如`bcm2708-rpi-b-plus.dtb`）合并，将会通过将I2S接口的状态更改为`okay`来启用I2S接口。 但是，如果您尝试使用以下命令编译此叠加层：

```
dtc -I dts -O dtb -o 2nd.dtbo 2nd-overlay.dts
```

你会看到一个错误提示：

```
Label or path i2s not found
```

这不应太出乎意料，因为没有引用基础`.dtb`或`.dts`文件来允许编译器找到`i2s`标签。

再试一次，这次使用原始示例并添加`-@`选项以允许未解析的引用：

```
dtc -@ -I dts -O dtb -o 1st.dtbo 1st-overlay.dts
```

如果`dtc`返回有关第三行的错误，则它没有覆盖工作所需的扩展名。 运行`sudo apt install device-tree-compiler`并重试-这次，编译应该成功完成。 注意，在内核树中还可以使用`scripts/dtc/dtc`作为合适的编译器，该编译器是在使用`dtbs` make target时构建的：

```
make ARCH=arm dtbs
```

转储DTB文件的内容以查看编译器生成的内容很有趣：

```
$ fdtdump 1st.dtbo

/dts-v1/;
// magic:           0xd00dfeed
// totalsize:       0x106 (262)
// off_dt_struct:   0x38
// off_dt_strings:  0xe8
// off_mem_rsvmap:  0x28
// version:         17
// last_comp_version:    16
// boot_cpuid_phys: 0x0
// size_dt_strings: 0x1e
// size_dt_struct:  0xb0

/ {
    compatible = "brcm,bcm2708";
    fragment@0 {
        target = <0xdeadbeef>;
        __overlay__ {
            status = "okay";
        };
    };
    __fixups__ {
        i2s = "/fragment@0:target:0";
    };
};
```

在文件结构的详细描述之后，是我们的片段。但是请仔细看-在我们写`＆i2s`的地方，现在写着`0xdeadbeef`，这表明发生了奇怪的事情。片段之后有一个新节点`__fixups__`。这包含一个属性列表，该属性列表将未解析符号的名称映射到片段中的单元格的路径列表，一旦找到了目标节点，该片段就需要使用目标节点的模范进行修补。在这种情况下，路径是`target`的值`0xdeadbeef`，但是片段可以包含其他未解析的引用，这将需要其他修复。

如果编写更复杂的片段，则编译器可能会生成另外两个节点：`__local_fixups__`和`__symbols__`。如果片段中的任何节点都有一个phandle，则前者是必需的，因为执行合并的程序将必须确保phandle号是连续且唯一的。但是，后者是如何处理未解析符号的关键。

在第1.3节中，它说“原始标签不会出现在编译输出中”，但是在使用`-@`开关时情况并非如此。相反，每个标签都会在`__symbols__`节点中产生一个属性，将标签映射到路径，就像`aliases`节点一样。实际上，该机制是如此相似，以至于在解析符号时，Raspberry Pi加载器将在没有`__symbols__`节点的情况下搜索“ aliases”节点。这很有用，因为通过提供足够的别名，我们可以允许使用较旧的`dtc`来构建基本DTB文件。

更新：内核中对[动态设备树](https://www.raspberrypi.org/documentation/configuration/device-tree.md#part3.5)的支持要求覆盖中的“本地修正”格式不同。为了避免新旧样式的叠加样式存在问题，并与其他叠加样式用户匹配，从4.4版开始，旧的“ name-overlay.dtb”命名方案已替换为“ name.dtbo”。覆盖层应该仅通过名称来引用，加载它们的固件或实用程序将附加适当的后缀。例如：

```
dtoverlay=awesome-overlay      # This is wrong
dtoverlay=awesome              # This is correct
```

### 2.2: 设备树参数

为了避免需要大量的设备树覆盖，并减少外围设备用户修改DTS文件的需求，Raspberry Pi加载程序支持一项新功能-设备树参数。 这允许使用命名参数对DT进行少量更改，类似于内核模块从`modprobe`和内核命令行接收参数的方式。 基本DTB和覆盖（包括HAT覆盖）可以暴露参数。

通过在根目录中添加`__overrides__`节点在DTS中定义参数。 它包含一些属性，这些属性的名称是所选的参数名称，其值是一个序列，该序列包括目标节点的phandle（对标签的引用）和指示目标属性的字符串； 支持字符串，整数（单元格）和布尔属性。

#### 2.2.1: 字符串

字符串的声明如下：

```
name = <&label>,"property";
```

其中`label`和`property`被适当的值替换。 字符串参数可以导致其目标属性增长，缩小或创建。

请注意，对称为`status`的属性进行了特殊处理。 non-zero/true/yes/on的值将转换为字符串`“okay”`，而zero/false/no/off将变为`“disabled”`。

#### 2.2.2: 整型

整型参数声明如下：

```
name = <&label>,"property.offset"; // 8-bit
name = <&label>,"property;offset"; // 16-bit
name = <&label>,"property:offset"; // 32-bit
name = <&label>,"property#offset"; // 64-bit
```

用适当的值替换`label`、`property`以及`offset`；相对于属性的开头以字节为单位指定偏移量（默认为十进制），并且前面的分隔符指示参数的大小。 与以前的实现方式相比，整数参数可以引用不存在的属性，也可以引用超出现有属性结尾的偏移量。

#### 2.2.3：布尔型

设备树将布尔值编码为零长度属性；如果存在，则该属性为true，否则为false。它们的定义如下：

```
boolean_property; //将'boolean_property'设置为true
```

请注意，通过不定义属性，为属性分配了`false`值。布尔参数声明如下：

```
name = <＆label>,"property?";
```

其中`label`和`property`被适当的值替换。布尔参数可以导致创建或删除属性。

#### 2.2.4：叠加/片段

所描述的DT参数机制具有许多限制，包括当使用参数时无法更改节点名称以及将任意值写入任意属性。克服其中一些限制的一种方法是有条件地包括或排除某些片段。

通过将`__overlay__`节点重命名为`__dormant__`，可以将片段从最终合并过程中排除（禁用）。扩展了参数声明语法，以允许否则为非法的零目标对象指示以下字符串包含片段或覆盖范围内的操作。到目前为止，已实施了四个操作：

```
+<n> //启用片段<n>
-<n> //禁用片段<n>
=<n> //如果分配的参数值为true，则启用片段<n>，否则将其禁用
!<n> //如果分配的参数值为false，则启用片段<n>，否则将其禁用
```

例子：

```
just_one    = <0>,"+1-2"; //启用1，停用2
conditional = <0>,"=3!4"; //启用3，如果值为true，则禁用4，
                          //否则禁用3，启用4。
```

i2c-mux覆盖使用此技术。

#### 2.2.5: 例子

以下是一些不同类型的属性的示例，并带有用于修改它们的参数：

```
/ {
    fragment@0 {
        target-path = "/";
        __overlay__ {

            test: test_node {
                string = "hello";
                status = "disabled";
                bytes = /bits/ 8 <0x67 0x89>;
                u16s = /bits/ 16 <0xabcd 0xef01>;
                u32s = /bits/ 32 <0xfedcba98 0x76543210>;
                u64s = /bits/ 64 < 0xaaaaa5a55a5a5555 0x0000111122223333>;
                bool1; // Defaults to true
                       // bool2 defaults to false
            };
        };
    };

    fragment@1 {
        target-path = "/";
        __overlay__ {
            frag1;
        };
    };

    fragment@2 {
        target-path = "/";
        __dormant__ {
            frag2;
        };
    };

    __overrides__ {
        string =      <&test>,"string";
        enable =      <&test>,"status";
        byte_0 =      <&test>,"bytes.0";
        byte_1 =      <&test>,"bytes.1";
        u16_0 =       <&test>,"u16s;0";
        u16_1 =       <&test>,"u16s;2";
        u32_0 =       <&test>,"u32s:0";
        u32_1 =       <&test>,"u32s:4";
        u64_0 =       <&test>,"u64s#0";
        u64_1 =       <&test>,"u64s#8";
        bool1 =       <&test>,"bool1?";
        bool2 =       <&test>,"bool2?";
        only1 =       <0>,"+1-2";
        only2 =       <0>,"-1+2";
        toggle1 =     <0>,"=1";
        toggle2 =     <0>,"=2";
        not1 =        <0>,"!1";
        not2 =        <0>,"!2";
    };
};
```

#### 2.2.6：具有多个目标的参数

在某些情况下，能够在设备树中的多个位置设置相同的值很方便。可以使用串联多个目标的方法来将多个目标添加到单个参数中，而不是笨拙地创建多个参数的方法：

```
    __overrides__ {
        gpiopin = <&w1>,"gpios:4",
                  <&w1_pins>,"brcm,pins:0";
        ...
    };
```
（示例取自`w1-gpio`叠加层）

请注意，甚至可以使用单个参数来定位不同类型的属性。您可以合理地将“启用”参数连接到`status`字符串，包含零或一的单元格以及适当的布尔属性。

#### 2.2.7：更多叠加示例

Raspberry Pi/Linux [GitHub存储库](https://github.com/raspberrypi/linux/tree/rpi-4.4.y/arch/arm/boot/dts/overlays)中托管着越来越多的叠加源文件。

## 第三部分：在树莓派中使用设备树

### 3.1：叠加层和config.txt

在Raspberry Pi上，加载程序（`start.elf`映像之一）的工作是将覆盖层与适当的基本设备树结合在一起，然后将完全解析的设备树传递给内核。基本设备树位于FAT分区中的`start.elf`旁边（从Linux启动），名为`bcm2708-rpi-b.dtb`，`bcm2708-rpi-b-plus.dtb`，`bcm2708-rpi-cm.dtb`和`bcm2709 -rpi-2-b.dtb`。请注意，模型A和A+将分别使用“ b”和“ b-plus”变体。此选择是自动的，并允许在各种设备中使用相同的SD卡映像。

请注意，DT和ATAG是互斥的。结果，将DT blob传递给不了解它的内核会导致启动失败。为了防止这种情况，加载程序会检查内核映像是否具有DT兼容性，这由`mkknlimg`实用程序添加的预告片标记；这可以在最近的内核源代码树的`scripts`目录中找到。假定没有预告片的任何内核均不具备DT功能。

如果没有DTB，则从rpi-4.4.y树（及更高版本）构建的内核将无法运行，因此从4.4版本开始，任何不带预告片的内核都被视为具有DT功能。您可以通过添加不带DTOK标志的预告片或通过将`device_tree=`放在config.txt中来覆盖它，但是如果它不起作用，请不要感到惊讶。 N.B.结果是，如果内核具有指示DT功能的预告片，则`device_tree=`将被忽略。

加载程序现在支持使用bcm2835_defconfig进行的构建，该版本选择上游的BCM2835支持。此配置将导致构建`bcm2835-rpi-b.dtb`和`bcm2835-rpi-b-plus.dtb`。如果这些文件是随内核一起复制的，并且内核已被最近的`mkknlimg`标记，则加载程序将默认尝试加载其中一个DTB。

为了管理设备树和覆盖，加载程序支持许多新的config.txt指令：

```
dtoverlay = acme-board
dtparam = foo = bar，level = 42
```

这将使加载程序在固件分区中查找`overlays/acme-board.dtbo`，Raspbian将其安装在/boot上。然后它将搜索参数`foo`和`level`，并为其指定指示的值。

加载程序还将搜索带有已编程EEPROM的附加HAT，并从那里加载支持的覆盖图。这种情况无需任何用户干预即可完成。

有几种方法可以表明内核正在使用设备树：

1. 在启动过程中，“Machine model：”内核消息具有特定于板的值，例如“Raspberry Pi 2 Model B”，而不是“ BCM2709”。
2. 一段时间后，可能还会有另一条内核消息显示“没有ATAG？”。 -这是预期的。
3. `/proc/device-tree`存在，并且包含与DT的节点和属性完全相同的子目录和文件。

使用设备树，内核将自动搜索并加载支持指定的已启用设备的模块。结果，通过为设备创建适当的DT覆盖，可以使设备用户免于编辑`/etc/ modules`的麻烦；所有配置都放在`config.txt`中，对于HAT，甚至不需要该步骤。但是请注意，诸如`i2c-dev`之类的分层模块仍需要显式加载。

不利的一面是，除非DTB要求，否则不会创建平台设备，因此不再需要将由于板支持代码中定义的平台设备而被加载的模块列入黑名单。实际上，当前的Raspbian图像没有黑名单文件。

### 3.2：DT参数

如上所述，DT参数是对设备配置进行小的更改的便捷方法。当前的基本DTB支持无需启用专用覆盖即可启用和控制板载音频，I2C，I2S和SPI接口的参数。在使用中，参数如下所示：

```
dtparam=audio=on,i2c_arm=on,i2c_arm_baudrate=400000,spi=on
```

请注意，可以将多个分配放在同一行上，但请确保您不超过80个字符的限制。

将来的默认`config.txt`可能包含以下部分：

```sh
# 取消注释部分或全部注释以启用可选的硬件接口
#dtparam=i2c_arm=on
#dtparam=i2s=on
#dtparam=spi=on
```

如果您有一个定义了一些参数的覆盖，则可以在后续行中指定它们，如下所示：

```
dtoverlay=lirc-rpi
dtparam=gpio_out_pin=16
dtparam=gpio_in_pin=17
dtparam=gpio_in_pull=down
```

或附加到叠加线，如下所示：

```
dtoverlay=lirc-rpi:gpio_out_pin=16,gpio_in_pin=17,gpio_in_pull=down
```

请注意，此处使用冒号（`:`)将叠加层名称与其参数分开，这是受支持的语法变体。

覆盖参数仅在作用域中，直到加载下一个覆盖。如果覆盖层和基础都导出了具有相同名称的参数，则覆盖层中的参数优先；为了清楚起见，建议您避免这样做。要公开由基本DTB导出的参数，请使用以下命令结束当前覆盖范围：

```
dtoverlay=
```

### 3.3：特定于电路板的标签和参数

Raspberry Pi板具有两个I2C接口。它们名义上是分开的：一个用于ARM，一个用于VideoCore（“ GPU”）。在几乎所有型号上，`i2c1`都属于ARM，而`i2c0`属于VC，用于控制相机和读取HAT EEPROM。但是，模型B有两个早期版本，其作用相反。

为了使所有Pi都能使用一组叠加层和参数，固件会创建一些特定于板的DT参数。这些是：

```
i2c/i2c_arm
i2c_vc
i2c_baudrate/i2c_arm_baudrate
i2c_vc_baudrate
```

这些是`i2c0`，`i2c1`，`i2c0_baudrate`和`i2c1_baudrate`的别名。建议仅在确实需要时才使用`i2c_vc`和`i2c_vc_baudrate`，例如，如果您正在编程HAT EEPROM。启用`i2c_vc`可以停止检测到Pi摄像机。

对于编写覆盖图的人员，相同的别名已应用于I2C DT节点上的标签。因此，您应该写：

```
fragment@0 {
    target = <&i2c_arm>;
    __overlay__ {
        status = "okay";
    };
};
```

使用数字变体的所有叠加层都将被修改为使用新的别名。

### 3.4：HAT和设备树

Raspberry Pi HAT是用于带有嵌入式EEPROM的“Plus”形（A +，B +或Pi 2 B）Raspberry Pi的附加板。 EEPROM包含使能电路板所需的任何DT覆盖层，并且该覆盖层还可以暴露参数。

HAT覆盖由固件在基本DTB之后自动加载，因此在加载任何其他覆盖或使用`dtoverlay=`终止覆盖范围之前，可以访问其参数。如果出于某种原因要抑制HAT叠加层的加载，请将`dtoverlay=`放在任何其他`dtoverlay`或`dtparam`指令之前。

### 3.5：动态设备树

从Linux 4.4开始，RPi内核支持动态加载覆盖和参数。兼容的内核管理一叠叠加层，这些叠加层应用在基本DTB的顶部。更改会立即反映在`/proc/device-tree`中，并且可能导致模块加载以及平台设备被创建和销毁。

上面的“堆栈”一词很重要-叠加层只能在堆栈的顶部添加和删除；更改堆栈下方的内容需要首先除去其顶部的所有内容。

有一些用于管理覆盖图的新命令：

#### 3.5.1 dtoverlay命令

`dtoverlay`是一个命令行实用程序，可在系统运行时加载和删除覆盖，以及列出可用的覆盖并显示其帮助信息：

```sh
pi@raspberrypi ~ $ dtoverlay -h
Usage:
  dtoverlay <overlay> [<param>=<val>...]
                           添加叠加层（带有参数）
  dtoverlay -r [<overlay>] 删除覆盖（按名称，索引或最后一个）
  dtoverlay -R [<overlay>] 从叠加层中删除（按名称，索引或全部）
  dtoverlay -l             列出活动的叠加层/参数
  dtoverlay -a             列出所有叠加层（标记为活动的）
  dtoverlay -h             显示此用法消息
  dtoverlay -h <overlay>   在叠加层上显示帮助
  dtoverlay -h <overlay> <param>..  或其参数
        其中<overlay>是dtparams的覆盖名称或'dtparam'
适用于大多数变体的选项：
    -d <dir>    指定overlays的替代位置
                (默认为/boot/overlays or /flash/overlays)
    -n          空运行 - 显示将执行的操作
    -v          详细操作
```

与`config.txt`等效项不同，覆盖层的所有参数都必须包含在同一命令行中-`dtparam`命令仅适用于基本DTB的参数。

需要注意两点：

更改内核状态（添加和删除内容）的命令变体需要root特权，因此您可能需要在命令前加上`sudo`前缀。

只能卸载在运行时应用的覆盖和参数-固件应用的覆盖或参数被“烘焙”，因此`dtoverlay`不会列出它，也无法将其删除。

#### 3.5.2 dtparam命令

`dtparam`创建的覆盖与在`config.txt`中使用dtparam指令具有相同的效果。在用法上，它与覆盖名称为 `-` 的`dtoverlay`等效，但有一些小区别：

1. `dtparam`将列出基本DTB的所有已知参数的帮助信息。仍然可以使用`dtparam -h`获得有关dtparam命令的帮助。
2. 当指示要删除的参数时，只能使用索引号（不能使用名称）。

#### 3.5.3 编写具有运行时能力的叠加层的准则

该区域的文档很少，但是这里有一些累积的技巧：

- 设备对象的创建或删除由添加或删除的节点触发，或者由节点的状态从禁用变为启用，反之亦然。当心-缺少“状态”属性意味着该节点已启用。

- 不要在片段内创建将覆盖基本DTB中现有节点的节点-内核将重命名新节点以使其唯一。如果要更改现有节点的属性，请创建一个针对它的片段。

- ALSA不会阻止其编解码器和其他组件在使用时被卸载。如果删除覆盖，则删除声卡仍在使用的编解码器会导致内核异常。实验发现，设备会按照覆盖中的片段顺序相反的顺序删除，因此将卡的节点放置在组件的节点之后可以有序关闭。

#### 3.5.4 注意事项

在运行时加载叠加层是内核的新增功能，到目前为止，尚无从用户空间执行此操作的方法。通过在命令后隐藏此机制的详细信息，目的是在其他内核接口标准化的情况下，使用户免受更改的影响。

- 一些覆盖在运行时比其他覆盖要好。设备树的某些部分仅在引导时使用-使用覆盖更改它们不会有任何影响。

- 应用或删除某些覆盖可能会导致意外的行为，因此应谨慎操作。这是它需要`sudo`的原因之一。

- 如果某些东西正在使用ALSA，则卸载ALSA卡的覆盖层可能会停顿-LXPanel音量滑块插件演示了这种效果。为了能够删除声卡的叠加层，`lxpanelctl`实用程序提供了两个新选项 - `alsastop`和`alsastart` - 分别在加载或卸载叠加层之前和之后从辅助脚本dtoverlay-pre和dtoverlay-post调用了这些选项。

- 删除覆盖不会导致已加载的模块被卸载，但是可能导致某些模块的引用计数降至零。两次运行`rmmod -a`将导致卸载未使用的模块。

- 覆盖层必须以相反的顺序除去。这些命令将允许您删除一个较早的命令，但是所有中间的命令将被删除并重新应用，这可能会带来意想不到的后果。

- 在运行时在`/clocks`节点下添加时钟不会导致注册新的时钟提供程序，因此`devm_clk_get`对于覆盖中创建的时钟将失败。

### 3.6：支持的叠加层和参数

由于在此处记录单个覆盖图非常耗时，因此请参阅`/boot/overlays`中覆盖图`.dtbo`文件旁边的[README](https://github.com/raspberrypi/firmware/blob/master/boot/overlays/README)。它通过添加和更改保持最新状态。

## 第4部分：故障排除和专业提示

### 4.1：调试

加载程序将跳过缺少的覆盖和错误的参数，但是如果存在严重错误，例如基本DTB丢失或损坏或覆盖合并失败，则加载程序将退回到非DT引导。如果发生这种情况，或者您的设置不符合预期，则值得检查加载程序中的警告或错误：

```sh
sudo vcdbg log msg
```

通过将`dtdebug=1`添加到`config.txt`可以启用额外的调试。

如果内核无法在DT模式下启动，**则可能是因为内核映像没有有效的尾部**。使用`knlinfo`检查一个，然后使用`mkknlimg`实用程序添加一个。这两个实用程序都包含在当前Raspberry Pi内核源代码树的`scripts`目录中。

您可以像这样创建人类可读的DT当前状态的表示形式：

```
dtc -I fs /proc/device-tree
```

这对于查看将覆盖层合并到基础树上的效果很有用。

如果内核模块未按预期加载，请检查`/etc/modprobe.d/raspi-blacklist.conf`中是否未将其列入黑名单；使用设备树时，不必将其列入黑名单。如果没有任何问题，您还可以通过在`/lib/modules/<version>/modules.alias`中搜索`compatible`值来检查模块是否导出了正确的别名。否则，您的驱动程序可能丢失：

```
.of_match_table = xxx_of_match,
```

要么：

```
MODULE_DEVICE_TABLE(of, xxx_of_match);
```

如果失败，则`depmod`失败或在目标文件系统上未安装更新的模块。

### 4.2：使用dtmerge和dtdiff测试覆盖

`dtoverlay`和`dtparam`命令旁边是一个实用程序，用于将覆盖图应用于DTB - `dtmerge`。要使用它，您首先需要获取基本的DTB，可以通过以下两种方式之一获得它：

a）从`/proc/device-tree`中的实时DT状态生成它：

```
dtc -I fs -O dtb -o base.dtb /proc/device-tree
```

这将包括您到目前为止已应用的所有叠加层和参数，无论是在`config.txt`中还是通过在运行时加载它们，都可能是您想要的，也可能不是。或者...

b）从/boot中的源DTB复制它。这将不包括覆盖和参数，但也将不包括固件的任何其他修改。为了允许测试所有覆盖，`dtmerge`实用程序将创建一些特定于板的别名（“ i2c_arm”等），但这意味着合并的结果将包括与原始DTB相比更多的差异。解决方案是使用dtmerge进行复制：

```
dtmerge /boot/bcm2710-rpi-3-b.dtb base.dtb-
```

（`"-"`表示缺少覆盖名称）。

现在，您可以尝试应用叠加层或参数：

```
dtmerge base.dtb merged.dtb - sd_overclock=62
dtdiff base.dtb merged.dtb
```

它将返回：

```
--- /dev/fd/63  2016-05-16 14:48:26.396024813 +0100
+++ /dev/fd/62  2016-05-16 14:48:26.396024813 +0100
@@ -594,7 +594,7 @@
                };

                sdhost@7e202000 {
-                       brcm,overclock-50 = <0x0>;
+                       brcm,overclock-50 = <0x3e>;
                        brcm,pio-limit = <0x1>;
                        bus-width = <0x4>;
                        clocks = <0x8>;
```

您还可以比较不同的叠加层或参数。

```
dtmerge base.dtb merged1.dtb /boot/overlays/spi1-1cs.dtbo
dtmerge base.dtb merged2.dtb /boot/overlays/spi1-2cs.dtbo
dtdiff merged1.dtb merged2.dtb
```

要得到：

```
--- /dev/fd/63  2016-05-16 14:18:56.189634286 +0100
+++ /dev/fd/62  2016-05-16 14:18:56.189634286 +0100
@@ -453,7 +453,7 @@

                        spi1_cs_pins {
                                brcm,function = <0x1>;
-                               brcm,pins = <0x12>;
+                               brcm,pins = <0x12 0x11>;
                                phandle = <0x3e>;
                        };

@@ -725,7 +725,7 @@
                        #size-cells = <0x0>;
                        clocks = <0x13 0x1>;
                        compatible = "brcm,bcm2835-aux-spi";
-                       cs-gpios = <0xc 0x12 0x1>;
+                       cs-gpios = <0xc 0x12 0x1 0xc 0x11 0x1>;
                        interrupts = <0x1 0x1d>;
                        linux,phandle = <0x30>;
                        phandle = <0x30>;
@@ -743,6 +743,16 @@
                                spi-max-frequency = <0x7a120>;
                                status = "okay";
                        };
+
+                       spidev@1 {
+                               #address-cells = <0x1>;
+                               #size-cells = <0x0>;
+                               compatible = "spidev";
+                               phandle = <0x41>;
+                               reg = <0x1>;
+                               spi-max-frequency = <0x7a120>;
+                               status = "okay";
+                       };
                };

                spi@7e2150C0 {
```

### 4.3：强制特定的设备树

如果您有默认DTB不支持的非常特殊的需求（尤其是人们尝试ARCH_BCM2835项目使用的纯DT方法），或者您只是想尝试编写自己的DT，您可以告诉加载程序以加载备用DTB文件，如下所示：

```
device_tree=my-pi.dtb
```

### 4.4：禁用设备树用法

由于切换到4.4内核并使用更多的上游驱动程序，因此在Pi内核中需要使用设备树。禁用DT使用的方法是添加：

```
device_tree=
```

到`config.txt`。但是，如果内核具有指示DT功能的`mkknlimg`预告片，则该指令将被忽略。

### 4.5：快捷方式和语法变体

加载程序了解一些快捷方式：

```
dtparam=i2c_arm=on
dtparam=i2s=on
```

可以缩短为：

```
dtparam=i2c_arm=on
dtparam=i2s=on
```

（`i2c`是`i2c_arm`的别名，并且假定`=on`）。它还仍然接受以下较长版本：`device_tree_overlay`和`device_tree_param`。

加载程序曾经接受使用空格和冒号作为分隔符，但是为简单起见，已不再支持它们，因此可以在不带引号的参数值中使用它们。

### 4.6：其他DT命令可在config.txt中使用

`device_tree_address`用于覆盖固件加载设备树的地址（不是dt-blob）。默认情况下，固件将选择合适的位置。

`device_tree_end`这为加载的设备树设置了（独占）限制。默认情况下，设备树可以增长到可用内存的末尾，几乎可以肯定这是必需的。

`dtdebug`如果非零，请为固件的设备树处理打开一些额外的日志记录。

`enable_uart`启用主/控制台UART（Pi 3上为ttyS0，否则为ttyAMA0-除非与诸如pi3-miniuart-bt的覆盖层交换）。如果主UART是ttyAMA0，则enable_uart默认为1（启用），否则默认为0（禁用）。这是因为必须停止更改ttyS0不可用的核心频率，因此`enable_uart=1`意味着core_freq = 250（除非force_turbo = 1）。在某些情况下，这会影响性能，因此默认情况下处于禁用状态。有关UART的更多详细信息，请参见[此处](https://www.raspberrypi.org/documentation/configuration/uart.md)

`overlay_prefix`指定要从中加载覆盖的子目录/前缀-默认为“overlays/”。注意尾随“/”。如果需要，您可以在最后的“/”之后添加一些内容，以便为每个文件添加一个前缀，尽管这不是必需的。

DT可以控制其他端口，有关更多详细信息，请参阅第3部分。