from configparser import ConfigParser
import os
import json
from argparse import ArgumentParser


def main(args):
    obj = {}
    config = ConfigParser()
    config.read(args.credentials)
    obj["MY_ACCESS_KEY"] = config.get("default", "aws_access_key_id",
                                      fallback="")
    obj["MY_SECRET_KEY"] = config.get("default", "aws_secret_access_key",
                                      fallback="")

    with open(args.output, "w") as out:
        json.dump(obj, out)
    os.chmod(args.output, 0o400)


if __name__ == '__main__':
    parser = ArgumentParser(description='Make a config json from AWS secrets')
    parser.add_argument('output', help='output path')
    parser.add_argument('--credentials', help='aws creds (default %(default)s)',
                        default=os.path.join(os.getenv("HOME"), ".aws",
                                             "credentials"))
    args = parser.parse_args()
    main(args)
