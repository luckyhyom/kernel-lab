
# 1단계 Linux VM 설치 & 모듈 생성 및 로드하기

### 학습 목표
- 커널 모듈 개발 환경 셋업 방법 익히기
- 모듈의 로드/언로드 라이프사이클 이해하기 (`insmod`/`rmmod`으로 모듈 삽입·제거)
- `printk`로 커널 로그 출력하고 `dmesg`로 확인
- “코드 수정 → 빌드 → 실행 → 디버그” 사이클을 몸으로 체득

## Multipass로 Ubuntu VM 띄우기
1. **Multipass 설치**
```bash
brew install multipass
```
2. **Ubuntu 22.04 VM 생성 및 접속**
```bash
multipass launch --name linux-dev 22.04 multipass shell linux-dev
```
이제 프롬프트가 `ubuntu@linux-dev:~$` 로 바뀔 거예요.


>**커널 모듈 · 드라이버 실습용**으로는 Docker보다는 “실제 리눅스 VM”을 띄우는 방법이 좋다.
>- Docker는 호스트 커널을 공유하기 때문에 모듈 로드 같은 커널 레벨 실험이 제한되고, VM은 독립 커널이어서 자유롭게 실습할 수 있어.
>- Docker **컨테이너 내부에서 모듈을 빌드**할 수는 있지만 **모듈을 실제로 로드(insmod)해서 동작을 확인이 가능해**


## VM 내부 환경 준비
터미널(`ubuntu@linux-dev:~$`)에서 차례대로 실행하세요:
1. **패키지 목록 갱신**
    ```sh
	  sudo apt update
	```
2. **필수 개발 패키지 설치**
    ```sh
    sudo apt install -y \
    build-essential \
    linux-headers-$(uname -r) \
    git \
    vim \
    iproute2 \
    bridge-utils \
    qemu-system-x86
	``` 
3. **작업 디렉토리 생성**
    ```sh
    mkdir -p ~/kernel-lab/hello cd ~/kernel-lab/hello
    ```
이 폴더가 1단계 실습 공간입니다.

### 실습
1. hello.c 작성
```c
#include <linux/init.h>
#include <linux/module.h>

static int __init hello_init(void) {
    printk(KERN_INFO ">> Hello, Kernel World!\n");
    return 0;
}

static void __exit hello_exit(void) {
    printk(KERN_INFO ">> Goodbye, Kernel World!\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Basic Hello World Kernel Module");
```

2. Makefile 작성
```makefile
obj-m := hello.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
```

3. 빌드 & **모듈 삽입**
```bash
make
sudo insmod hello.ko
```

4. 로그 확인 (기대 출력: Hello)
```bash
dmesg | tail -n 10
```


5. **모듈 언로그** & 로그 확인 (기대 출력: Goodbye)
```bash
sudo rmmod hello
dmesg | tail -n 10
```

출력 결과
```bash
[ 1441.152267] >> Hello, Kernel World!
[ 1485.907315] >> Goodbye, Kernel World!
```


### 생성된 파일들
```bash
hello.c          # 소스 코드  
Makefile         # 빌드 스크립트  
hello.o          # hello.c → 컴파일된 오브젝트 (ELF 형식)  
hello.mod.c      # modpost가 생성한 모듈 메타데이터 C 코드  
hello.mod.o      # hello.mod.c → 컴파일된 오브젝트 (메타정보 포함)  
hello.ko         # 최종 커널 모듈 (hello.o + hello.mod.o 링크 결과)  
modules.order    # 모듈 설치 순서를 나열한 텍스트  
```
- **hello.o**
    - `hello.c`를 `gcc -c`로 컴파일한 결과
    - 실제 init/exit 함수가 담긴 오브젝트 파일
- **hello.mod.c**
    - 빌드 과정 중 `modpost` 단계에서 자동 생성되는 C 파일
    - 모듈 이름, 의존 심볼, 버전 정보, `__this_module` 구조체 등을 정의
    - 직접 작성하진 않지만, 모듈 정보를 커널에 알려 주는 역할
- **hello.mod.o**
    - `hello.mod.c`를 컴파일한 오브젝트
    - 모듈 메타데이터(버전, 심볼 테이블)를 담고 있음
- **hello.ko**
    - `hello.o`와 `hello.mod.o`를 링크해서 만든 **커널 로드용 객체**
    - `insmod`/`modprobe`로 이 파일을 커널에 올리게 됨
- **modules.order**
    - 이 모듈을 `modules_install` 할 때 어떤 순서로 설치해야 하는지 적은 텍스트 파일
    - 외부 모듈을 한꺼번에 설치할 때 유용

### 왜 이런 파일들이 생기나?
1. **Kbuild 시스템**
    - `obj-m := hello.o` 로 지정하면
        1. `hello.c` → `hello.o`
        2. `modpost` → `hello.mod.c` → `hello.mod.o`
        3. 두 오브젝트를 링크 → `hello.ko`
    - 이 세 단계로 모듈 객체와 메타데이터를 분리·생성
2. **메타데이터 분리의 장점**
    - 모듈 버전 관리, 의존성 체크, 심볼 검증 등을 `modpost`가 자동화
    - 개발자는 오직 `hello.c`만 신경 쓰면 되고,
    - 모듈 정보(라이선스, 심볼 버전) 관리가 일관되게 이뤄짐

### 읽을거리
- [https://yohda.tistory.com/entry/LINUXKERNEL-Linux-kernel-headers](https://yohda.tistory.com/entry/LINUXKERNEL-Linux-kernel-headers)


---
>기타 질문들

### 리눅스 직접 설치 vs VM 설치
- “리눅스가 제공하는 순수 소프트웨어 기능”(시스템 콜, 모듈 삽입·제거, 파일 시스템, 네트워크 스택 등)은 **VM과 베어메탈 모두 동일**하게 사용할 수 있습니다.
- 일반적인 커널 모듈·드라이버 실습(메모리 매핑, MMIO, TAP/Virtio-net 등)에는 VM도 전혀 문제 없음.
- 반대로 “실물 하드웨어 레지스터”나 “특수 장치”를 직접 제어해 봐야 한다면 베어메탈(또는 해당 보드를 연결한 크로스 개발 환경)이 필요합니다.

### Docker 컨테이너 vs 리눅스 VM
- **Docker 컨테이너**
    - **OS 레벨 가상화**: 컨테이너들은 **호스트 커널**을 그대로 사용
    - **커널 권한 제한**: 대부분의 컨테이너 환경에서는 보안상 `insmod`/`modprobe` 권한이 잠겨 있어, 컨테이너 내에서 커널 모듈을 로드할 수 없음
    - **커널 버전 고정**: 컨테이너 이미지는 루트 파일시스템만 격리될 뿐, 실제 커널은 호스트(또는 VM)의 커널을 그대로 공유
- **리눅스 VM**
    - **하드웨어 가상화**: VM은 자체적으로 “완전한 리눅스 커널”을 부팅
    - **커널 제어권 보유**: VM 안에서는 호스트와는 별개의 커널을 직접 빌드·로드·디버그할 수 있음
    - **커널 모듈 실습 가능**: `insmod hello.ko` → `dmesg` 확인까지 완전 제어

→ **결론**: Docker 컨테이너는 호스트 커널을 공유하기 때문에 그 위에 새로운 모듈을 로드할 수 없고, 따라서 커널 모듈 실습에는 부적합해요.

### Multipass vs 일반 VM(또는 베어메탈 설치)

| 항목          | Multipass VM                  | 전통적 VM (VirtualBox/Vmware)  | 베어메탈 설치         |
| ----------- | ----------------------------- | --------------------------- | --------------- |
| **생성/삭제**   | `multipass launch/ delete`    | Vagrant/VBox GUI/API        | OS 설치 → 삭제 번거로움 |
| **일시 정지**   | `multipass stop <name>`       | VBoxManage suspend / resume | 지원 안 됨          |
| **데이터 영속성** | VM 디스크 이미지(가상 디스크) 내부에 영구 저장  | VM 디스크에 영구 저장               | 로컬 디스크에 직접 저장   |
| **하드웨어 매핑** | 호스트의 CPU/RAM/디스크 일부를 가상화하여 사용 | 동일                          | 실제 하드웨어 직접 사용   |
| **퍼포먼스**    | 네이티브 VM 수준                    | 네이티브 VM 수준                  | 하드웨어 최대 성능      |
| **관리 편의성**  | 간단한 CLI(한 줄 명령)               | GUI·스크립트 등 다양               | 물리 장비 관리 필요     |
- **Multipass**는 내부적으로 하이퍼바이저(예: HyperKit, QEMU/KVM)를 써서 VM을 띄우고,  
    `multipass stop` 하면 게스트 OS를 안전하게 중지(pause)시키고,  
    `multipass start` 하면 다시 부팅 없이 바로 재개할 수 있어요.
- **안에 설치한 파일**(패키지, 소스, 모듈)은 VM 디스크 이미지에 남아 있고, VM을 삭제(`multipass delete`)하지 않는 한 영구적으로 보존됩니다.
- **베어메탈 설치**와 달리 하드웨어 펌웨어(UEFI/BIOS)나 파티션 설정은 건드리지 않고, 순수하게 가상 하드웨어 안에서만 모든 작업이 이뤄져요.

### 커널 모듈이란?
- **동적으로 커널에 추가(로드)·제거(언로드)**할 수 있는 객체 코드(.ko 파일)야.
- “모듈”을 쓰면 **전체 커널을 재빌드하거나 재부팅하지 않고** 필요한 기능(드라이버, 파일 시스템, 네트워크 프로토콜 등)을 바로 붙였다 뗄 수 있어.
- 내부적으로는 **특별한 ELF 포맷**으로 빌드된 오브젝트 파일에,
	- `module_init`/`module_exit` 함수 포인터
	- 심볼 버전·의존성 정보
	- 라이선스·설명 같은 메타데이터가 함께 들어있어.
- `insmod`로 로드하면 커널 주소 공간에 붙고, `rmmod`로 언로드하면 깔끔히 분리돼 사라져.

### 지금 만든 hello.ko는 드라이버인가?
- **아니**, 지금 만든 것은 **하드웨어 제어 로직이 없는 “테스트용 샘플 모듈”**이야.
- ‘드라이버(Driver)’는 특정 장치(NIC, 디스크, USB 등)를 커널이 제어·관리하도록 해 주는 모듈을 뜻하는데,
    - hello.ko는 **단지 로드/언로드 시 메시지를 찍는 역할**만 하거든.
- 드라이버가 되려면
    1. `register_chrdev`나 `pci_register_driver` 같은 **장치 등록 API**를 호출하고
    2. `file_operations`나 `net_device_ops` 같은 **콜백 테이블**을 채워서
    3. 실제 **하드웨어 레지스터를 읽고 쓰는 코드**를 구현해야 해.
- 따라서 hello.ko는 “커널 모듈”이지만, “장치 드라이버”는 아니라고 구분하면 돼!

### 어쨋든 커널 모듈을 만든다는 것은 하드웨어를 제어할수있다는거지?

맞아. **커널 모듈을 만든다는 건 곧 “커널 공간에서 실행되는 코드”를 추가한다는 뜻이고, 이 코드는 사용자 공간보다 훨씬 낮은 레벨에서 하드웨어를 직접 제어할 수 있는 권한을 갖게 돼.**

1. **커널 공간 실행**
	- 사용자 공간 프로그램은 `read()`, `write()` 같은 시스템 콜을 통해서만 제한적으로 하드웨어에 접근할 수 있지만,
	- 커널 모듈은 커널 내부에서 동작하므로 **메모리 매핑(MMIO), I/O 포트, 인터럽트 처리, DMA 설정** 등 하드웨어 제어에 필요한 모든 권한을 가짐.
2. **필수 API 제공**
	- **MMIO 매핑**:
	```c
	void __iomem *base = ioremap(phys_addr, size); writel(value, base + OFFSET); u32 status = readl(base + STATUS_OFFSET);
	```
	- **I/O 포트 접근**:
	```c
	outb(data, port_addr); u8 val = inb(port_addr);
	```            
	- **인터럽트 등록**:
	```c
	request_irq(irq_line, irq_handler, IRQF_SHARED, "my_dev", dev);
	```
	- **DMA 버퍼 할당**:
	```c
	dma_addr_t dma_handle; void *buf = dma_alloc_coherent(dev, buf_size, &dma_handle, GFP_KERNEL);
	```
3. **장치 드라이버 모델과 연계**
    - 예를 들어 **PCI 디바이스 드라이버**라면 `struct pci_driver`를 등록하고
	```c
	static struct pci_driver my_pci_driver = {     .name = "my_pci",     .id_table = my_pci_ids,     .probe = my_probe,     .remove = my_remove, }; pci_register_driver(&my_pci_driver);
	```
	- `probe()` 안에서 위 API들을 사용해 레지스터에 접근하고 하드웨어 초기화를 수행.


