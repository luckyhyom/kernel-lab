### 1) 학습 목표
- 문자(Character) 디바이스 모델 구조 이해
- 사용자 공간 ↔ 커널 공간 간 I/O 경로 체득

### 2) 배울 수 있는 점
- `alloc_chrdev_region`으로 **major/minor** 동적 할당
- `cdev_init`/`cdev_add`를 통한 **cdev 등록**
- `file_operations` 구조체에 `open`/`read`/`write`/`release` 콜백 연결
- `/dev`에 **mknod**으로 디바이스 파일 생성·사용법

### 3) 왜 도움이 되는지
- 모든 디바이스 드라이버의 **공통 패턴**을 익히면,  
    네트워크·블록·USB 등 다른 드라이버로 확장할 때 기반이 됩니다.

```bash
# 1. 모듈 로드
sudo insmod char_dev.ko

# 2. 로그 확인: 할당된 major/minor 번호 확인
sudo dmesg | tail -n 5
# 출력 예시:
# [ 1234.567890] mychardev: registered major=240 minor=0

# 3. /dev 디바이스 파일 생성
#    (위에서 확인한 major 번호를 <MAJOR>에 넣으세요)
sudo mknod /dev/mychardev c <MAJOR> 0  
sudo chmod 666 /dev/mychardev

# 4. 쓰기 테스트
echo "hello kernel" > /dev/mychardev
sudo dmesg | tail -n 5
# 출력 예시:
# [ 1235.000000] mychardev: write 12 bytes: hello kernel

# 5. 읽기 테스트
cat /dev/mychardev
sudo dmesg | tail -n 5
# 출력 예시:
# hello kernel
# [ 1235.100000] mychardev: read 12 bytes

# 6. 모듈 언로드
sudo rmmod char_dev
sudo dmesg | tail -n 5
# 출력 예시:
# [ 1236.000000] mychardev: unregistered

```
