#ifndef SERENITY_UTILS_HPP
#define SERENITY_UTILS_HPP

#include <vector>

template <typename T>
T flattenListsInsideVector(std::vector<T> _vectorOfContainers) {
  T products;
  for (T items : _vectorOfContainers) {
    for (auto item : items) {
      products.push_back(item);
    }
  }
  return products;
}

#endif  // SERENITY_UTILS_HPP
