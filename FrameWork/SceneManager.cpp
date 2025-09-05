#include "SceneManager.h"

void SceneManager::OnInit()
{
}

void SceneManager::OnRelease()
{
}

void SceneManager::AddContent(int key, IContent* content)
{
    // 키를 못찼을 경우 맵에 콘텐츠 추가하기
    if (contentMap.find(key) == contentMap.end())
        contentMap.insert(std::make_pair(key, content));  
}

void SceneManager::ChangeContent(int key)
{
    // 적용된 콘텐츠가 있을 경우 한차례 Release(삭제)를 해준다.
    if (curContent != nullptr)
        curContent->OnRelease();

    // 
    if (contentMap.find(key) != contentMap.end())
    {
        // 적용된 콘텐츠를 변경하구, 초기화를 진행한다.
        curContent = contentMap[key];
        curContent->OnInit();
    }
}
