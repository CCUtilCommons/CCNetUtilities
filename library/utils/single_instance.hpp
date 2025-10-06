#pragma once

template <typename SingleInstanceType>
class SingleInstance {
public:
	static SingleInstanceType& instance() {
		static SingleInstanceType instance;
		return instance;
	}

protected:
	SingleInstance() = default;
	virtual ~SingleInstance() = default;

private:
	SingleInstance(const SingleInstance&) = delete;
	SingleInstance& operator=(const SingleInstance&) = delete;
	SingleInstance(SingleInstance&&) = delete;
	SingleInstance& operator=(SingleInstance&&) = delete;
};
