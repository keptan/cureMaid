# cureMaid
server client program for managing a transactional anime girl database 
![](https://u.teknik.io/vmas5.jpg)

currenly working through SQL bindings
can do selects into structured bindings : ) 
```c++
for(const auto [name, age] : db.SELECT<std::string, int>("SELECT * from ages"))
std::cout << name ' ' << age << '\n';
```
