#include "Player.h"

unsigned int Player::numCreaturesInHand()
{
	return creaturesInHand.size();
}

Creature* Player::getCreatureInHand(int i)
{
	return creaturesInHand[i];
}

void Player::addCreatureToHand(Creature *c)
{
	creaturesInHand.push_back(c);
}

void Player::removeCreatureFromHand(int i)
{
	vector<Creature*>::iterator curCreature = creaturesInHand.begin();
	while(i > 0 && curCreature != creaturesInHand.end())
	{
		i--;
		curCreature++;
	}

	creaturesInHand.erase(curCreature);
}

