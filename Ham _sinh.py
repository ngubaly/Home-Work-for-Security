def middle_square(seed, n):
    # Tính độ dài của seed
    length = len(str(seed))
    # Tính giá trị của 10^length
    power = 10 ** length
    
    for i in range(n):
        # Bình phương seed
        square = seed * seed
        # Lấy 2n chữ số ở giữa của square
        square_str = str(square)
        middle = (len(square_str) - length) // 2
        result_str = square_str[middle:middle+length]
        # Chuyển kết quả thành số
        result = int(result_str)
        # Tính lại seed cho vòng lặp tiếp theo
        seed = result
        result_binary = bin(result)
        # Đưa ra kết quả
        print(result_binary[2:], end = '')
        
middle_square(121, 200)
