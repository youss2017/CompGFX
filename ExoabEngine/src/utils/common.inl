
// TEMPLATE FUNCTIONS FOR common.hpp

template <typename T>
bool WriteBinaryFile(const char *file_path, const std::vector<T> &buffer)
{
    if (buffer.size() == 0)
    {
        Utils::CreateEmptyFile(file_path);
        return true;
    }
    FILE *handle = fopen(file_path, "wb");
    if (!handle)
        return false;
    fwrite(buffer.data(), buffer.size() * sizeof(T), 1, handle);
    fclose(handle);
    return true;
}

template <typename T>
bool WriteBinaryFile(const std::string &file_path, const std::vector<T> &buffer)
{
    return WriteBinaryFile<T>(file_path.c_str(), buffer);
}

template <typename T>
bool WriteBinaryFile(const std::filesystem::path &file_path, const std::vector<T> &buffer)
{
    std::string path = file_path.c_str();
    return WriteBinaryFile<T>(path.c_str(), buffer);
}

template <typename T>
std::vector<T> CombineVectors(std::vector<T> &A, std::vector<T> &B)
{
    std::vector<T> C;
    C.reserve(A.size() + B.size());
    for (const auto &a : A)
        C.push_back(a);
    for (const auto &b : B)
        C.push_back(b);
    return C;
}